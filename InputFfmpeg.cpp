/*
 * File:   InputFfmpeg.cpp
 * Author: tobias
 *
 * Created on 21. Oktober 2013, 12:06
 * 
 * originally from peerstreamer (Streamers/input-grapes.c)
 */

#include <iostream>
#include <string>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
    // FFMPEG
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

    // GRAPES
#include <chunkbuffer.h>
#include <chunkiser.h>
#ifdef __cplusplus
}
#endif

#include "InputFfmpeg.hpp"
#include "Threads.hpp"


using namespace std;

InputFfmpeg::InputFfmpeg() {
}

InputFfmpeg::InputFfmpeg(const InputFfmpeg& orig) {
}

InputFfmpeg::~InputFfmpeg() {
}

bool InputFfmpeg::open(string filename) {
    //this->inputStream = input_stream_open(filename.c_str(), &this->period, this->inOptions);
    this->inputStream = input_stream_open(filename.c_str(), &this->period, "chunkiser=avf,media=av");
    if (this->inputStream == NULL) {
        fprintf(stderr, "Cannot open input file %s\n", filename.c_str());
        return false;
    }

    if (this->period == 0) {
        this->inFDs = input_get_fds(this->inputStream);
    } else {
        this->inFDs = NULL;
    }
    this->currentChunkId = 0;

    return true;
}

void InputFfmpeg::close() {
    input_stream_close(this->inputStream);
}

bool InputFfmpeg::generateChunk() {
#ifdef DEBUG
    //    fprintf(stderr, "Generating Chunk %d started\n", this->currentChunkId);
#endif
    int res;
    chunk *c = (chunk*) malloc(sizeof (chunk));

    c->id = this->currentChunkId;
    c->attributes = NULL;
    c->attributes_size = 0;
    res = chunkise(this->inputStream, c);

    if (res < 0) {
        fprintf(stderr, "Error in input!\n");
        Threads::stopThreads = true;
        return false;
    }

    res = cb_add_chunk(Streamer::chunkBuffer, c);
    if (res < 0) {
        free(c->data);
        c->data = NULL;
        free(c->attributes);
        c->attributes = NULL;
        free(c);
        c = NULL;
        return false;
    } else {
        // get new chunk buffer size
        cb_get_chunks(Streamer::chunkBuffer, &Streamer::chunkBufferSize);

        // create new chunkIDSet because first one could not be removed
        Network *network = Network::getInstance();
        chunkID_set_free(Streamer::chunkIDSet);
        Streamer::chunkIDSet = NULL;
        Streamer::chunkIDSet = network->chunkBufferToBufferMap();
    }

#ifdef DEBUG
    int n;
    chunk *chunksInBuffer = cb_get_chunks(Streamer::chunkBuffer, &n);
    if (this->currentChunkId % 10 == 0) {
        // print only every 10 chunks
        fprintf(stdout, "Generating Chunk %d finished: %d chunks in buffer (first: %d, last %d)\n",
                this->currentChunkId,
                Streamer::chunkBufferSize,
                chunksInBuffer[0].id,
                chunksInBuffer[n - 1].id
                );
    }
#endif

    ++this->currentChunkId;

    return true;
}

const int *InputFfmpeg::getInFDs() {
    return this->inFDs;
}

useconds_t InputFfmpeg::getPauseBetweenChunks() {
    return (useconds_t) this->period;
}

uint8_t *InputFfmpeg::getSecuredDataChunk(nodeID *remote, int chunkId) {
    // nothing to do here, nothing to crypt
    return NULL;
}

uint8_t *InputFfmpeg::getSecuredDataLogin(nodeID *remote) {
    // nothing to do here, nothing to crypt
    return NULL;
}

bool InputFfmpeg::securedDataEnabledChunk() {
    return false;
}

bool InputFfmpeg::securedDataEnabledLogin() {
    return false;
}
