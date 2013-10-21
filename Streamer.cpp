/*
 * File:   Streamer.cpp
 * Author: tobias
 *
 * Created on 17. Oktober 2013, 12:19
 */

#include <iostream>
#include <string>

#include "Streamer.hpp"
#include "Threads.hpp"

#include <net_helper.h>

using namespace std;

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
    cout << "called Streamer::init" << endl;
#endif
    struct nodeID *myID;
    Network n = new Network();
    this->network = &n;
    char *my_addr = this->network->createInterface(this->configInterface);

    if (my_addr == NULL) {
        fprintf(stderr, "Cannot find network interface %s\n", this->configInterface);

        return NULL;
    }

    myID = net_helper_init(my_addr, this->configPort);
    if (myID == NULL) {
        fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr, this->configPort);
        free(my_addr);

        return NULL;
    }
    free(my_addr);
    topInit(myID);

    return myID;
}

void Streamer::startThreads() {
#ifdef DEBUG
    fprintf(stdout, "Called Streamer::loop\n");
#endif
    pthread_t generate_thread, receive_thread, gossiping_thread, distributing_thread;

    period = csize;
    chunks_per_period = chunks;
    s = s1;

    pset = peerset_init(0);
    sigInit(s, pset); // TODO
    source_init(fname, s); // TODO
    pthread_mutex_init(&cb_mutex, NULL);
    pthread_mutex_init(&topology_mutex, NULL);
    pthread_create(&receiveDataThread, NULL, Threads::receiveData, NULL); // Thread for receiving data
    pthread_create(&gossiping_thread, NULL, Threads::sendTopology, NULL); // Thread for sharing the 
    pthread_create(&generate_thread, NULL, Threads::forgeChunk, NULL);
    pthread_create(&distributing_thread, NULL, Threads::sendChunk, NULL);

    pthread_join(generate_thread, NULL);
    pthread_join(receive_thread, NULL);
    pthread_join(gossiping_thread, NULL);
    pthread_join(distributing_thread, NULL);
}
