/*
 * File:   Threads.cpp
 * Author: tobias
 *
 * Created on 18. Oktober 2013, 13:11
 */

#include <iostream>
#include <string>
#include <stdio.h>
using namespace std;

#ifdef __cplusplus
extern "C" {
#endif
// GRAPES
#include <grapes_msg_types.h>
#include <net_helper.h>
#ifdef __cplusplus
}
#endif

#include "Network.hpp"
#include "Input.hpp"
#include "Streamer.hpp"
#include "Threads.hpp"

bool Threads::stopThreads = false;
int Threads::chunks_per_period = 1;
int Threads::gossipingPeriod = 500000;
int Threads::done = false;
nodeID *Threads::s = NULL;

struct mutexes {
    Threads *t;
};

Threads::Threads() {
}

Threads::Threads(const Threads& orig) {
}

Threads::~Threads() {
}

void Threads::startThreads(Streamer *s) {
#ifdef DEBUG
    fprintf(stdout, "Called Threads::startThreads\n");
#endif

    this->streamer = s;

    //source_loop(fname,             my_sock,           period * 1000, multiply);
    //source_loop(const char *fname, struct nodeID *s1, int csize,     int chunks)

    pthread_t generateChunksThread, receiveDataThread, sendTopologyThread, sendChunkThread;
    Network *network = Network::getInstance();

    //period = csize; // size of chunks
    //chunks_per_period = chunks;

    mutexes mut = {this};

    Streamer::peerSet = peerset_init(0);
    network->initializeSignaling(this->streamer->getSocket(), Streamer::peerSet);
    this->streamer->initializeSource();

    // initialize mutexes
    pthread_mutex_init(&this->chunkBufferMutex, NULL);
    pthread_mutex_init(&this->topologyMutex, NULL);

    // create threads
    pthread_create(&receiveDataThread, NULL, Threads::receiveData, (void *) &mut); // Thread for receiving data
    pthread_create(&sendTopologyThread, NULL, Threads::sendTopology, (void *) &mut); // Thread for sharing the topology of the p2p network
    pthread_create(&generateChunksThread, NULL, Threads::generateChunks, (void *) &mut); // Thread for generating chunks
    pthread_create(&sendChunkThread, NULL, Threads::sendChunk, (void *) &mut); // Thread for sending the chunks

    // join threads
    pthread_join(generateChunksThread, NULL);
    pthread_join(receiveDataThread, NULL);
    pthread_join(sendTopologyThread, NULL);
    pthread_join(sendChunkThread, NULL);
}

void *Threads::receiveData(void *mut) {
    mutexes *m = (mutexes *) mut;
    Threads *t = m->t;

    Network *network = Network::getInstance();
    while (!Threads::stopThreads) {
        int numberOfReceivedBytes;
        struct nodeID *remote;
        static uint8_t buffer[BUFFSIZE];

        numberOfReceivedBytes = recv_from_peer(s, &remote, buffer, BUFFSIZE);
        switch (buffer[0] /* Message Type */) {
            case MSG_TYPE_TOPOLOGY:
                pthread_mutex_lock(&t->topologyMutex);
                network->updatePeers(Streamer::peerSet, buffer, numberOfReceivedBytes);
                pthread_mutex_unlock(&t->topologyMutex);
                break;
            case MSG_TYPE_CHUNK:
                fprintf(stderr, "Some dumb peer pushed a chunk to me!\n");
                break;
            default:
                fprintf(stderr, "Unknown Message Type %x\n", buffer[0]);
        }
        nodeid_free(remote);
    }

    return NULL;
}

void *Threads::sendTopology(void *mut) {
    mutexes *m = (mutexes *) mut;
    Threads *t = m->t;

    Network *network = Network::getInstance();

    pthread_mutex_lock(&t->topologyMutex);
    network->updatePeers(Streamer::peerSet, NULL, 0);
    pthread_mutex_unlock(&t->topologyMutex);
    while (!Threads::stopThreads) {
        pthread_mutex_lock(&t->topologyMutex);
        network->updatePeers(Streamer::peerSet, NULL, 0);
        pthread_mutex_unlock(&t->topologyMutex);
        usleep(Threads::gossipingPeriod * 10);
    }

    return NULL;
}

void *Threads::generateChunks(void *mut) {
    mutexes *m = (mutexes *) mut;
    Threads *t = m->t;

    Input *input = Input::getInstance();
    int64_t d;

    while (!Threads::stopThreads) {
        pthread_mutex_lock(&t->chunkBufferMutex);
        input->generateChunk(&d);
        pthread_mutex_unlock(&t->chunkBufferMutex);
        usleep(d);
    }

    return NULL;
}

void *Threads::sendChunk(void *mut) {
    mutexes *m = (mutexes *) mut;
    Threads *t = m->t;

    Network *network = Network::getInstance();
    int chunk_period = Threads::gossipingPeriod / Threads::chunks_per_period;

    while (!Threads::stopThreads) {
        pthread_mutex_lock(&t->topologyMutex);
        pthread_mutex_lock(&t->chunkBufferMutex);
        network->sendChunkToPeerSet(t->streamer->peerSet);
        pthread_mutex_unlock(&t->chunkBufferMutex);
        pthread_mutex_unlock(&t->topologyMutex);
        usleep(chunk_period);
    }

    return NULL;
}
