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
    bool initializeSource();

    string getConfigFilename();
    void setConfigFilename(string configFilename);
    string getConfigInterface();
    void setConfigInterface(string configInterface);
    int getConfigPort();
    void setConfigPort(int configPort);
    Network* getNetwork();
    void setNetwork(Network* network);
    nodeID* getSocket();
    void setSocket(nodeID* socket);
    static ChunkBuffer *cb;

    static InputDescription *inputDescription;
    static int chunkBufferSize;
    static peerset *peerSet;
private:


    bool initializeStream(int size);

    // configuration
    string configFilename;
    string configInterface;
    int configPort;

    nodeID *socket;
    Network *network;
    int metadata;

};

#endif	/* STREAMER_HPP */
