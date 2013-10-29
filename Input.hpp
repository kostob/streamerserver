/*
 * File:   Input.hpp
 * Author: tobias
 *
 * Created on 21. Oktober 2013, 12:06
 */

#ifndef INPUT_HPP
#define	INPUT_HPP

#define HEADER_REFRESH_PERIOD 50

#include <iostream>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif
// FFMPEG
#include <libavformat/avformat.h>

// GRAPES
#include <chunk.h>
#ifdef __cplusplus
}
#endif

#include "structs.hpp"
#include "Streamer.hpp"

using namespace std;

class Input { // class is a singleton
public:
    static Input* getInstance();
    InputDescription *open(string filename);
    bool generateChunk(int64_t *delta);
private:
    Input();
    Input(const Input& orig);
    virtual ~Input();
    bool openSource(string filename, int *period);
    void closeSource();
    long get(chunk *c);
    uint8_t *chunkise(int id, int *size, uint64_t *ts);

    AVFormatContext *stream;
    int audioStreamIndex;
    int videoStreamIndex;
    int64_t lastTimestamp;
    int framesSinceGlobalHeaders;
    InputDescription *inputDescription;
};

#endif	/* INPUT_HPP */

