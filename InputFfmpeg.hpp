/*
 * File:   InputFfmpeg.hpp
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

#include "Streamer.hpp"
#include "InputFactory.hpp"
#include "InputInterface.hpp"


using namespace std;

class InputFfmpeg : public InputInterface {
public:
    InputFfmpeg();
    InputFfmpeg(const InputFfmpeg& orig);
    virtual ~InputFfmpeg();

    // methods from interface
    bool open(string filename);
    bool generateChunk();
    void close();
    uint8_t *getSecuredDataChunk(nodeID *remote, int chunkId);
    uint8_t *getSecuredDataLogin(nodeID *remote);
    bool securedDataEnabledChunk();
    bool securedDataEnabledLogin();
    useconds_t getPauseBetweenChunks();

    const int *getInFDs();

private:
    input_stream *inputStream;
    int currentChunkId;
    const int *inFDs;
    int period;
    char inOptions[1024];
    char *inOptionsPointer;
};

#endif	/* INPUT_HPP */

