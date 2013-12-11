/*
 * File:   Threads.cpp
 * Author: tobias
 *
 * Created on 18. Oktober 2013, 13:11
 */

#include <iostream>
#include <string>
#include <stdio.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif
    // GRAPES
#include <grapes_msg_types.h>
#include <net_helper.h>
#include <trade_msg_ha.h>
#ifdef __cplusplus
}
#endif

#include "Network.hpp"
#include "InputFfmpeg.hpp"
#include "Streamer.hpp"
#include "Threads.hpp"

using namespace std;

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

    pthread_t generateChunkThread, receiveDataThread, sendTopologyThread, sendChunkThread, offerChunksThread;

    //period = csize; // size of chunks
    //chunks_per_period = chunks;

    mutexes mut = {this};

    psample_parse_data(Streamer::peersampleContext, NULL, 0);

    // initialize mutexes
    pthread_mutex_init(&this->chunkBufferMutex, NULL);
    pthread_mutex_init(&this->topologyMutex, NULL);
    pthread_mutex_init(&this->peerChunkMutex, NULL);

    // create threads
    pthread_create(&receiveDataThread, NULL, Threads::receiveData, (void *) &mut); // Thread for receiving data
    pthread_create(&sendTopologyThread, NULL, Threads::sendTopology, (void *) &mut); // Thread for sharing the topology of the p2p network
    pthread_create(&generateChunkThread, NULL, Threads::generateChunk, (void *) &mut); // Thread for generating chunks
    pthread_create(&sendChunkThread, NULL, Threads::sendChunk, (void *) &mut); // Thread for sending the chunks
    pthread_create(&offerChunksThread, NULL, Threads::offerChunks, (void *) &mut); // Thread for sending the chunks

    // join threads
    pthread_join(generateChunkThread, NULL);
    pthread_join(receiveDataThread, NULL);
    pthread_join(sendTopologyThread, NULL);
    pthread_join(sendChunkThread, NULL);
    pthread_join(offerChunksThread, NULL);
}

void *Threads::receiveData(void *mut) {
#ifdef DEBUG
    fprintf(stdout, "Thread started Threads::receiveData\n");
#endif
    mutexes *m = (mutexes *) mut;
    Threads *t = m->t;

    Network *network = Network::getInstance();

    while (!Threads::stopThreads) {
        int numberOfReceivedBytes;
        nodeID *remote;
        nodeID *owner;
        static uint8_t buffer[BUFFSIZE];

        numberOfReceivedBytes = recv_from_peer(t->streamer->getSocket(), &remote, buffer, BUFFSIZE);
        char remoteAddress[256];
        node_addr(remote, remoteAddress, 256);

#ifdef DEBUG
        fprintf(stdout, "DEBUG: Received message of %d bytes from %s\n", numberOfReceivedBytes, remoteAddress);
#endif

        switch (buffer[0] /* Message Type */) {
            case MSG_TYPE_TOPOLOGY:
            {
#ifdef DEBUG
                fprintf(stdout, "DEBUG: Received topology message from peer %s!\n", remoteAddress);
#endif
                pthread_mutex_lock(&t->topologyMutex);
                psample_parse_data(Streamer::peersampleContext, buffer, numberOfReceivedBytes);
                pthread_mutex_unlock(&t->topologyMutex);

                break;
            }
            case MSG_TYPE_CHUNK:
            {
#ifdef DEBUG
                fprintf(stdout, "DEBUG: Some dumb peer (%s) pushed a chunk to me!\n", remoteAddress);
#endif
                break;
            }
            case MSG_TYPE_SIGNALLING:
            {
                int maxDeliver;
                uint16_t transId;
                int chunkToSend;
                signaling_type signalingType;
                int result;
                ChunkIDSet *chunkIDSetReceived;

                //                char remoteAddress2[256];
                //                node_addr(remote, remoteAddress2, 256);
                //                fprintf(stdout, "MSG_TYPE_SIGNALING: Remote 2 is now: %s\n", remoteAddress2);

#ifdef DEBUG
                fprintf(stdout, "MSG_TYPE_SIGNALING: Calling parseSignaling\n");
#endif
                result = parseSignaling(buffer + 1, numberOfReceivedBytes - 1, &owner, &chunkIDSetReceived, &maxDeliver, &transId, &signalingType);

                //                char remoteAddress3[256];
                //                node_addr(remote, remoteAddress3, 256);
                //                fprintf(stdout, "MSG_TYPE_SIGNALING: Remote 3 is now: %s\n", remoteAddress3);

                //                char addrOwner[256];
                //                node_addr(owner, addrOwner, 256);
                //                fprintf(stdout, "MSG_TYPE_SIGNALING: OWNER is %s\n", addrOwner);

                if (owner) {
                    nodeid_free(owner);
                }

                //                char remoteAddress4[256];
                //                node_addr(remote, remoteAddress4, 256);
                //                fprintf(stdout, "MSG_TYPE_SIGNALING: Remote 4 is now: %s\n", remoteAddress4);

                switch (signalingType) {
                    case sig_accept:
                    {
#ifdef DEBUG
                        fprintf(stdout, "DEBUG: 1) Message ACCEPT: peer (%s) accepted %d chunks\n", remoteAddress, chunkID_set_size(chunkIDSetReceived));
#endif
                        pthread_mutex_lock(&t->chunkBufferMutex);
                        pthread_mutex_lock(&t->peerChunkMutex);
                        int i, chunkID;
                        for (i = 0; i < chunkID_set_size(chunkIDSetReceived); ++i) {
                            chunkID = chunkID_set_get_chunk(chunkIDSetReceived, i);

                            // add to PeerChunk
                            network->addToPeerChunk(remote, chunkID);
                        }
                        pthread_mutex_unlock(&t->peerChunkMutex);
                        pthread_mutex_unlock(&t->chunkBufferMutex);

                        break;
                    }
                    case sig_deliver:
                    {
                        fprintf(stdout, "DEBUG: 1) Message DELIVER: peer wants me to deliver %d chunks\n", chunkID_set_size(chunkIDSetReceived));
                        pthread_mutex_lock(&t->peerChunkMutex);
                        int i, chunkID;
                        for (i = 0; i < chunkID_set_size(chunkIDSetReceived); ++i) {
                            chunkID = chunkID_set_get_chunk(chunkIDSetReceived, i);

                            // add to PeerChunk
                            network->addToPeerChunk(remote, chunkID);
                        }
                        pthread_mutex_unlock(&t->peerChunkMutex);
                        break;
                    }
                    case sig_ack:
                        break;
                    case sig_offer: // Peer offers x chunks... I'm the server, I don't need chunks...
                        break;
                    case sig_request: // peer requests x chunks
                    {
#ifdef DEBUG
                        fprintf(stdout, "DEBUG: 1) Message REQUEST: peer (%s) requests %d chunks\n", remoteAddress, chunkID_set_size(chunkIDSetReceived));
#endif
                        //printChunkID_set(Streamer::chunkIDSet);
                        chunkToSend = chunkID_set_get_earliest(chunkIDSetReceived);

                        pthread_mutex_lock(&t->chunkBufferMutex);
                        pthread_mutex_lock(&t->peerChunkMutex);

                        if (network->addToPeerChunk(remote, chunkToSend) == true) { // add peer/chunk to a list, for sending in sendChunk thread
                            // we have the chunk, we could send it to the remote peer
                            ChunkIDSet *chunkIDSetRemote = chunkID_set_init("size=1,type=bitmap");
#ifdef DEBUG
                            fprintf(stdout, "DEBUG: 2) Message REQUEST: Deliver only earliest chunk #%d to %s\n", chunkToSend, remoteAddress);
#endif
                            chunkID_set_add_chunk(chunkIDSetRemote, chunkToSend);
                            deliverChunks(remote, chunkIDSetRemote, transId++); // tell remote that this chunk could be delivered
                        } else {
                            // the requested chunk is too old, send current buffermap
#ifdef DEBUG
                            fprintf(stdout, "DEBUG: 2) Message REQUEST: the requested chunk is too old, send current buffermap to %s\n", remoteAddress);
#endif
                            ChunkIDSet *chunkIDSetRemote = network->chunkBufferToBufferMap();
                            sendBufferMap(remote, t->streamer->getSocket(), chunkIDSetRemote, chunkID_set_size(chunkIDSetRemote), transId);
                        }
                        pthread_mutex_unlock(&t->peerChunkMutex);
                        pthread_mutex_unlock(&t->chunkBufferMutex);

                        break;
                    }
                    case sig_send_buffermap: // peer sent its buffermap
                    {
#ifdef DEBUG
                        fprintf(stdout, "DEBUG: 1) Message SEND_BMAP: received buffer map of %d chunks from %s\n", chunkID_set_size(chunkIDSetReceived), remoteAddress);
#endif
                        break;
                    }
                    case sig_request_buffermap: // peer requests my buffer map
                    {
#ifdef DEBUG
                        char foo[256];
                        node_addr(remote, foo, 256);
                        fprintf(stdout, "DEBUG: 1) Message REQUEST_BMAP: %s requested my buffer map\n", foo);
#endif
                        pthread_mutex_lock(&t->chunkBufferMutex);
                        pthread_mutex_lock(&t->topologyMutex);
                        sendBufferMap(remote, t->streamer->getSocket(), Streamer::chunkIDSet, chunkID_set_size(Streamer::chunkIDSet), transId++);
                        pthread_mutex_unlock(&t->topologyMutex);
                        pthread_mutex_unlock(&t->chunkBufferMutex);
                        break;
                    }
                    case sig_request_secured_data_chunk:
                    {
#ifdef DEBUG
                        char remoteAddress[256];
                        node_addr(remote, remoteAddress, 256);
                        fprintf(stdout, "DEBUG: 1) Message REQUEST_SECURED_DATA: %s requested secure data\n", remoteAddress);
#endif
                        pthread_mutex_lock(&t->chunkBufferMutex);
                        if (t->streamer->getInput()->securedDataEnabledChunk()) {
                            chunk *c;
                            c = (chunk*) malloc(sizeof(chunk));

                            for (int i = 0; i < chunkID_set_size(chunkIDSetReceived); ++i) {
                                c->id = chunkID_set_get_chunk(chunkIDSetReceived, i);
                                c->data = t->streamer->getInput()->getSecuredDataChunk(remote, chunkID_set_get_chunk(chunkIDSetReceived, i));
                                sendSecuredChunk(remote, c, transId++);
                            }
                        }
                        pthread_mutex_unlock(&t->chunkBufferMutex);

                        break;
                    }
                    case sig_request_secured_data_login:
                    {
#ifdef DEBUG
                        char remoteAddress[256];
                        node_addr(remote, remoteAddress, 256);
                        fprintf(stdout, "DEBUG: 1) Message REQUEST_SECURED_DATA_LOGIN: %s requested secure data for login\n", remoteAddress);
#endif
                        if (t->streamer->getInput()->securedDataEnabledLogin()) {
                            chunk *c;
                            c = (chunk*) malloc(sizeof(chunk));
                            c->data = t->streamer->getInput()->getSecuredDataLogin(remote);
                            sendSecuredChunkLogin(remote, c, transId++);
                        }
                        break;
                    }
                }
                break;
            }
            default:
            {
                fprintf(stderr, "Unknown Message Type \"%x\" from %s\n", buffer[0], remoteAddress);
                break;
            }
        }
        nodeid_free(remote);
    }

    return NULL;
}

void *Threads::sendTopology(void *mut) {
#ifdef DEBUG
    fprintf(stdout, "Thread started::sendTopology\n");
#endif
    mutexes *m = (mutexes *) mut;
    Threads *t = m->t;
    char addr[256];

    while (!Threads::stopThreads) {
        const nodeID * const *neighbours;
        int numberOfNeighbours;

        pthread_mutex_lock(&t->topologyMutex);
        psample_parse_data(Streamer::peersampleContext, NULL, 0);
#ifdef DEBUG
        neighbours = psample_get_cache(Streamer::peersampleContext, &numberOfNeighbours);
        fprintf(stdout, "I have %d neighbours:\n", numberOfNeighbours);
        for (int i = 0; i < numberOfNeighbours; i++) {
            node_addr(neighbours[i], addr, 256);
            fprintf(stdout, "\t%d: %s\n", i, addr);
        }
        fflush(stdout);
#endif
        pthread_mutex_unlock(&t->topologyMutex);
        sleep(1); // send topology every second
    }

    return NULL;
}

void *Threads::generateChunk(void *mut) {
#ifdef DEBUG
    fprintf(stdout, "Thread started::generateChunk\n");
#endif
    mutexes *m = (mutexes *) mut;
    Threads *t = m->t;

    while (!Threads::stopThreads) {
        pthread_mutex_lock(&t->chunkBufferMutex);
        t->streamer->getInput()->generateChunk();
        pthread_mutex_unlock(&t->chunkBufferMutex);

        usleep(t->streamer->getInput()->getPauseBetweenChunks());
    }

    return NULL;
}

void *Threads::sendChunk(void *mut) {
#ifdef DEBUG
    fprintf(stdout, "Thread started Threads::sendChunk\n");
#endif
    mutexes *m = (mutexes *) mut;
    Threads *t = m->t;

    Network *network = Network::getInstance();
    int chunk_period = Threads::gossipingPeriod / Threads::chunks_per_period;

    while (!Threads::stopThreads) {
        pthread_mutex_lock(&t->topologyMutex);
        pthread_mutex_lock(&t->chunkBufferMutex);
        pthread_mutex_lock(&t->peerChunkMutex);
        network->sendChunksToPeers();
        pthread_mutex_unlock(&t->peerChunkMutex);
        pthread_mutex_unlock(&t->chunkBufferMutex);
        pthread_mutex_unlock(&t->topologyMutex);
        usleep(chunk_period);
    }

    return NULL;
}

void *Threads::offerChunks(void* mut) {
#ifdef DEBUG
    fprintf(stdout, "Thread started Threads::offerChunks\n");
#endif
    mutexes *m = (mutexes *) mut;
    Threads *t = m->t;

    Network *network = Network::getInstance();

    while (!Threads::stopThreads) {
        pthread_mutex_lock(&t->topologyMutex);
        pthread_mutex_lock(&t->chunkBufferMutex);
        pthread_mutex_lock(&t->peerChunkMutex);
        network->offerChunksToPeers();
        pthread_mutex_unlock(&t->peerChunkMutex);
        pthread_mutex_unlock(&t->chunkBufferMutex);
        pthread_mutex_unlock(&t->topologyMutex);
        usleep(5 * 1000 * 100); // 0.5 sec
    }

    return NULL;
}
