/*
 * File:   Streamer.cpp
 * Author: tobias
 *
 * Created on 17. Oktober 2013, 12:19
 */

#include <iostream>
#include <string>

// GRAPES
#ifdef __cplusplus
extern "C" {
#endif
#include <net_helper.h>
#include <peerset.h>
#include <peersampler.h>
#include <chunk.h>
#include <trade_msg_ha.h>
#ifdef __cplusplus
}
#endif

#include "Streamer.hpp"
#include "Threads.hpp"
#include "InputFactory.hpp"

using namespace std;

int Streamer::chunkBufferSize = 0;
int Streamer::chunkBufferSizeMax = 100;
PeerSet *Streamer::peerSet = NULL;
ChunkBuffer *Streamer::chunkBuffer = NULL;
ChunkIDSet *Streamer::chunkIDSet = NULL;
PeerChunk *Streamer::peerChunks = NULL;
int Streamer::peerChunksSize = 0;
psample_context *Streamer::peersampleContext = NULL;

Streamer::Streamer() {
    this->configFilename = "";
    this->configInterface = "lo0";
    this->configPort = 55555;
    this->configPeersample = "protocol=cyclon";
    this->configChunkBuffer = "size=100,time=now"; // size must be same value as chunkBufferSizeMax
    this->configChunkIDSet = "size=100,type=bitmap"; // size must be same value as chunkBufferSizeMax
    this->configInputType = "ffmpeg";
}

Streamer::Streamer(const Streamer& orig) {
}

Streamer::~Streamer() {
}

void Streamer::parseCommandLineArguments(int argc, char* argv[]) {
#ifdef DEBUG
    //cout << "called Streamer::parseCommandLineArguments" << endl;
#endif
    string arg;
    for (int i = 1; i < argc; ++i) {
        // TODO: check if there is an argument i + 1
        arg = argv[i];

        // filename
        if (arg.compare("-f") == 0) {
            this->configFilename = argv[i + 1];
        }

        // interface
        if (arg.compare("-I") == 0) {
            this->configInterface = argv[i + 1];
        }

        // port
        if (arg.compare("-p") == 0) {
            this->configPort = atoi(argv[i + 1]);
        }

        // input type
        if (arg.compare("-t") == 0) {
            this->configInputType = argv[i + 1];
        }

        ++i;
    }
}

bool Streamer::init() {
#ifdef DEBUG
    //fprintf(stdout, "Called Streamer::init\n");
#endif
    // check if a filename was entered
    if (this->configFilename.empty()) {
        fprintf(stderr, "No filename entered: please use -f <filename>\n");
        return false;
    }

    // create the interface for connection
    this->network = Network::getInstance();
    string my_addr = this->network->createInterface(this->configInterface);

    if (my_addr.empty()) {
        fprintf(stderr, "Cannot find network interface %s\n", this->configInterface.c_str());
        return false;
    }

    // initialize net helper
    this->socket = net_helper_init(my_addr.c_str(), this->configPort, "");
    if (this->socket == NULL) {
        fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr.c_str(), this->configPort);
        return false;
    }

    // initialize PeerSampler
    Streamer::peersampleContext = psample_init(this->socket, NULL, 0, this->configPeersample.c_str());
    if (Streamer::peersampleContext == NULL) {
        fprintf(stderr, "Error while initializing the peer sampler\n");
        return false;
    }

    // initialize ChunkBuffer
    Streamer::chunkBuffer = cb_init(this->configChunkBuffer.c_str());
    if (Streamer::chunkBuffer == NULL) {
        fprintf(stderr, "Error while initializing chunk buffer\n");
        return false;
    }

    // initialize PeerSet
    Streamer::peerSet = peerset_init("size=0");
    if (Streamer::peerSet == NULL) {
        fprintf(stderr, "Error while initializing peerset\n");
        return false;
    }

    // initialize chunk delivery
    if (chunkDeliveryInit(this->socket) < 0) {
        fprintf(stderr, "Error while initializing chunk delivery\n");
        return false;
    }

    // init chunk signaling
    if (chunkSignalingInit(this->socket) == -1) {
        fprintf(stderr, "Error while initializing chunk signaling\n");
        return false;
    }

    // init chunkid set
    Streamer::chunkIDSet = chunkID_set_init(configChunkIDSet.c_str()); // ,type=bitmap"
    if (Streamer::chunkIDSet == NULL) {
        fprintf(stderr, "Error while initializing chunkid set\n");
        return false;
    }

    // init source
    this->input = InputFactory::createInput(configInputType);
    if (this->input != NULL) {
        if (input->open(this->configFilename) == false) {
            return false;
        }
    } else {
        return false;
    }

    return true;
}

string Streamer::getConfigFilename() {
    return configFilename;
}

void Streamer::setConfigFilename(string configFilename) {
    this->configFilename = configFilename;
}

string Streamer::getConfigInterface() {
    return configInterface;
}

void Streamer::setConfigInterface(string configInterface) {
    this->configInterface = configInterface;
}

int Streamer::getConfigPort() {
    return configPort;
}

void Streamer::setConfigPort(int configPort) {
    this->configPort = configPort;
}

nodeID * Streamer::getSocket() {
    return socket;
}

void Streamer::setSocket(nodeID * socket) {
    this->socket = socket;
}

InputInterface * Streamer::getInput() {
    return this->input;
}
