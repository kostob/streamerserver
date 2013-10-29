/*
 * File:   Streamer.cpp
 * Author: tobias
 *
 * Created on 17. Oktober 2013, 12:19
 */

#include <iostream>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif
// GRAPES
#include <net_helper.h>
#include <peerset.h>
#include <topmanager.h>
#include <chunk.h>
#include <trade_msg_ha.h>
#ifdef __cplusplus
}
#endif

#include "Streamer.hpp"
#include "Threads.hpp"

using namespace std;

InputDescription *Streamer::inputDescription = NULL;
int Streamer::chunkBufferSize = 0;
peerset *Streamer::peerSet = NULL;
ChunkBuffer *Streamer::cb = NULL;

Streamer::Streamer() {
    this->configFilename = "";
    this->configInterface = "lo0";
    this->configPort = 6666;
}

Streamer::Streamer(const Streamer& orig) {
}

Streamer::~Streamer() {
}

void Streamer::parseCommandLineArguments(int argc, char* argv[]) {
#ifdef DEBUG
    cout << "called Streamer::parseCommandLineArguments" << endl;
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

        ++i;
    }
}

bool Streamer::init() {
#ifdef DEBUG
    fprintf(stdout, "Called Streamer::init\n");
#endif
    // check if a filename was entered
    if(this->configFilename.empty()) {
        fprintf(stderr, "No filename entered: please use -f <filename>\n");
        cout << "No filename entered: please use -f <filename>" << endl;
        return false;
    }
    
    this->network = Network::getInstance();
    string my_addr = this->network->createInterface(this->configInterface);

    if (my_addr.empty()) {
        fprintf(stderr, "Cannot find network interface %s\n", this->configInterface.c_str());
        return false;
    }

    this->socket = net_helper_init(my_addr.c_str(), this->configPort, "");
    if (this->socket == NULL) {
        fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr.c_str(), this->configPort);
        //free(my_addr); // not needed for std::string

        return false;
    }

    // free(my_addr); // not needed for std::string

    //int resultTopInit = topInit(this->socket, &this->metadata, sizeof(this->metadata), NULL);
    int resultTopInit = topInit(this->socket, &this->metadata, sizeof(this->metadata), "protocol=cyclon");
    if (resultTopInit < 0) {
        fprintf(stderr, "Error while initialization of topology manager\n");

        return false;
    }

    return true;
}

bool Streamer::initializeSource() {
    Input *input = Input::getInstance();
    this->inputDescription = input->open(this->configFilename);
    if (this->inputDescription == NULL) {
        return false;
    }


    this->initializeStream(1);
    return true;
}

bool Streamer::initializeStream(int size) {
    char conf[32];

    Streamer::chunkBufferSize = size;

    sprintf(conf, "size=%d", Streamer::chunkBufferSize);
    cb = cb_init(conf);
    chunkDeliveryInit(this->socket);
    
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

Network* Streamer::getNetwork() {
    return network;
}

void Streamer::setNetwork(Network* network) {
    this->network = network;
}

nodeID* Streamer::getSocket() {
    return socket;
}

void Streamer::setSocket(nodeID* socket) {
    this->socket = socket;
}
