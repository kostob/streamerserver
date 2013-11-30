/*
 * File:   Streamer.hpp
 * Author: tobias
 *
 * Created on 17. Oktober 2013, 12:19
 */

#ifndef STREAMER_HPP
#define	STREAMER_HPP

#include <iostream>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif
    // GRAPES
#include <chunkbuffer.h>
#include <peerset.h>
#include <peersampler.h>
#ifdef __cplusplus
}
#endif

#include "Network.hpp"
#include "Input.hpp"


using namespace std;

class Streamer {
public:
    Streamer();
    Streamer(const Streamer& orig);
    virtual ~Streamer();
    void parseCommandLineArguments(int argc, char* argv[]);
    bool init();

    string getConfigFilename();
    void setConfigFilename(string configFilename);
    string getConfigInterface();
    void setConfigInterface(string configInterface);
    int getConfigPort();
    void setConfigPort(int configPort);
    nodeID* getSocket();
    void setSocket(nodeID* socket);

    static ChunkBuffer *chunkBuffer;
    static int chunkBufferSize;
    static int chunkBufferSizeMax;
    static PeerSet *peerSet;
    static ChunkIDSet *chunkIDSet;
    static PeerChunk *peerChunks;
    static int peerChunksSize;
    static psample_context *peersampleContext;
private:
    // configuration
    string configFilename;
    string configInterface;
    int configPort;
    string configPeersample;
    string configChunkBuffer;
    string configChunkIDSet;

    nodeID *socket;
    Network *network;
    int metadata;
};

#endif	/* STREAMER_HPP */
