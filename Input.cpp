/*
 * File:   Input.cpp
 * Author: tobias
 *
 * Created on 21. Oktober 2013, 12:06
 */

#include <iostream>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif
// FFMPEG
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

// GRAPES
#include <chunkbuffer.h>
#ifdef __cplusplus
}
#endif

#include "structs.hpp"
#include "Input.hpp"


using namespace std;

Input* Input::getInstance() {
    static Input i;
    return &i;
}

Input::Input() {
}

Input::Input(const Input& orig) {
}

Input::~Input() {
}

InputDescription *Input::open(string filename) {
    InputDescription *res;
    timeval tv;

    res = (InputDescription*) malloc(sizeof (InputDescription));
    if (res == NULL) {
        return NULL;
    }
    res->id = 0;
    gettimeofday(&tv, NULL);
    res->start_time = tv.tv_usec + tv.tv_sec * 1000000ULL;
    res->first_ts = 0;
    //res->s = this->openSource(filename, &res->interframe);
    //if (res->s == NULL) {

    if (!this->openSource(filename, &res->interframe)) {
        free(res);
        res = NULL;
    }

    return res;
}

bool Input::openSource(string filename, int *period) {
    avcodec_register_all();
    av_register_all();

    // open input file
    if (avformat_open_input(&this->stream, filename.c_str(), NULL, NULL) != 0) {
        fprintf(stderr, "Error opening %s\n", filename.c_str());
        return false;
    }

    // find stream information
    if (avformat_find_stream_info(this->stream, NULL) < 0) {
        fprintf(stderr, "Cannot find stream information for %s\n", filename.c_str());
        return false;
    }

    // debugging output to show whats inside the file
    av_dump_format(this->stream, 0, filename.c_str(), 0);

    this->videoStreamIndex = -1;
    this->audioStreamIndex = -1;
    this->lastTimestamp = 0;
    this->framesSinceGlobalHeaders = 0;

    // walk through the file to find the first video stream
    for (unsigned int i = 0; i < this->stream->nb_streams; ++i) {
        if (this->videoStreamIndex == -1 && this->stream->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            this->videoStreamIndex = i;
            // do we need the following fprintf?
            fprintf(stderr, "Video Frame Rate = %d/%d --- Period: %lld\n",
                    this->stream->streams[i]->r_frame_rate.num,
                    this->stream->streams[i]->r_frame_rate.den,
                    (long long int) av_rescale(1000000, this->stream->streams[i]->r_frame_rate.den, this->stream->streams[i]->r_frame_rate.num));

            // TODO: what is this statement doing and what do we do with "period"?
            *period = av_rescale(1000000, this->stream->streams[i]->r_frame_rate.den, this->stream->streams[i]->r_frame_rate.num);
        }
        if (this->audioStreamIndex == -1 && this->stream->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            this->audioStreamIndex = i;
        }
    }

    if (this->videoStreamIndex == -1) {
        fprintf(stderr, "No video stream found in %s\n", filename.c_str());
        return false;
    }

    return true;
}

void Input::closeSource() {
    avformat_close_input(&this->stream);
    free(this->stream);
}

bool Input::generateChunk(int64_t* delta) {
    int res;
    struct chunk c;

    *delta = this->get(&c);
    if (*delta < 0) {
        fprintf(stderr, "Error in input!\n");
        exit(-1);
    }
    if (c.data == NULL) {
        return false;
    }
    res = cb_add_chunk(Streamer::cb, &c); // TODO
    if (res < 0) {
        free(c.data);
        free(c.attributes);
    }

    return true;
}

int64_t Input::get(chunk* c) {
    struct timeval now;
    int64_t delta;

    c->data = this->chunkise(Streamer::inputDescription->id, &c->size, &c->timestamp);
    if (c->size == -1) {
        return -1;
    }
    if (c->data) {
        c->id = Streamer::inputDescription->id++;
    }
    c->attributes_size = 0;
    c->attributes = NULL;
    if (Streamer::inputDescription->first_ts == 0) {
        Streamer::inputDescription->first_ts = c->timestamp;
    }
    delta = c->timestamp - Streamer::inputDescription->first_ts + Streamer::inputDescription->interframe;
    gettimeofday(&now, NULL);
    delta = delta + Streamer::inputDescription->start_time - now.tv_sec * 1000000ULL - now.tv_usec;

#ifdef DEBUG
    fprintf(stdout, "Delta: %lld\n", (long long int) delta);
    fprintf(stdout, "Generate Chunk[%d] (TS: %llu): %s\n", c->id, (long long int) c->timestamp, c->data);
#endif

    return delta > 0 ? delta : 0;
}

uint8_t *Input::chunkise(int id, int *size, uint64_t *timestamp) {
    AVPacket packet;
    uint8_t *data; // place where the data of the frame will be stored
    bool addHeader = false;

    if (av_read_frame(this->stream, &packet) < 0) {
        fprintf(stderr, "AVPacket read failed\n");
        *size = -1;

        return NULL;
    }

    if ((packet.flags & AV_PKT_FLAG_KEY) == 0) {
        addHeader = true;
    }
    //addHeader = (packet.flags & AV_PKT_FLAG_KEY) != 0;
    if (addHeader) {
        this->framesSinceGlobalHeaders++;
        if (this->framesSinceGlobalHeaders == HEADER_REFRESH_PERIOD) {
            this->framesSinceGlobalHeaders = 0;
            addHeader = true;
        }
    }

    // calculate size of the chunk
    if (addHeader) {
        *size = packet.size + this->stream->streams[packet.stream_index]->codec->extradata_size;
    } else {
        *size = packet.size;
    }

    data = (uint8_t*) malloc(*size);
    if (data == NULL) {
        fprintf(stderr, "Could not allocate space for chunk %d", id);
        *size = -1;
        av_free_packet(&packet);

        return NULL;
    }

    // write data for chunk in "data" field
    if (addHeader && this->stream->streams[packet.stream_index]->codec->extradata_size) {
        memcpy(data, this->stream->streams[packet.stream_index]->codec->extradata, this->stream->streams[packet.stream_index]->codec->extradata_size);
        memcpy(data + this->stream->streams[packet.stream_index]->codec->extradata_size, packet.data, packet.size);
    } else {
        memcpy(data, packet.data, packet.size);
    }

    *timestamp = av_rescale_q(packet.dts, this->stream->streams[packet.stream_index]->time_base, AV_TIME_BASE_Q);
    this->lastTimestamp = *timestamp;

    av_free_packet(&packet);

    return data;
}
