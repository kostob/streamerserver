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

// GRAPES
#include <grapes_msg_types.h>
#include <net_helper.h>


#include "Threads.hpp"
#include "Network.hpp"

Threads::Threads() {
}

Threads::Threads(Network *network) {
    this->network = network;
}

Threads::Threads(const Threads& orig) {
}

Threads::~Threads() {
}

static void Threads::receiveData(void *network) {
    Network *network = (Network*) network;
    while (!Threads::stopThreads) {
        int lenght;
        struct nodeID *remote;
        static uint8_t buffer[BUFFSIZE];

        lenght = recv_from_peer(s, &remote, buffer, BUFFSIZE);
        switch (buffer[0] /* Message Type */) {
            case MSG_TYPE_TOPOLOGY:
                pthread_mutex_lock(&topology_mutex);
                network->updatePeers(peerSet, buffer, lenght);
                pthread_mutex_unlock(&topology_mutex);
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

static void Threads::sendTopology(void *network) {
    Network *network = (Network*) network;
    int gossiping_period = period * 10;

    pthread_mutex_lock(&topology_mutex);
    network->updadatePeers(peerSet, NULL, 0);
    pthread_mutex_unlock(&topology_mutex);
    while (!Threads::stopThreads) {
        pthread_mutex_lock(&topology_mutex);
        update_peers(peerSet, NULL, 0);
        pthread_mutex_unlock(&topology_mutex);
        usleep(gossiping_period);
    }

    return NULL;
}

static void Threads::forgeChunk(void *network) {
    Network *network = (Network*) network;
    suseconds_t d;

    while (!Threads::stopThreads) {
        pthread_mutex_lock(&cb_mutex);
        generated_chunk(&d);
        pthread_mutex_unlock(&cb_mutex);
        usleep(d);
    }

    return NULL;
}

static void Threads::sendChunk(void *network) {
    Network *network = (Network*) network;
    int chunk_period = period / chunks_per_period;

    while (!Threads::stopThreads) {
        pthread_mutex_lock(&topology_mutex);
        pthread_mutex_lock(&cb_mutex);
        send_chunk(peerSet);
        pthread_mutex_unlock(&cb_mutex);
        pthread_mutex_unlock(&topology_mutex);
        usleep(chunk_period);
    }

    return NULL;
}
