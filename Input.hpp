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
#include <chunkiser.h>
#ifdef __cplusplus
}
#endif

#include "structs.hpp"
#include "Streamer.hpp"

using namespace std;

class Input { // class is a singleton
public:
    static Input* getInstance();
    bool open(string filename);
    bool generateChunk();
    const int *getInFDs();
    int getPeriod();
private:
    Input();
    Input(const Input& orig);
    virtual ~Input();
    void closeSource();

    input_stream *inputStream;
    int currentChunkId;
    const int *inFDs;
    int period;
    char inOptions[1024];
    char *inOptionsPointer;
};

#endif	/* INPUT_HPP */

