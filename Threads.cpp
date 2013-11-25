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
#include <netinet/in.h>
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

    pthread_t generateChunkThread, receiveDataThread, sendTopologyThread, sendChunkThread;
    Network *network = Network::getInstance();

    //period = csize; // size of chunks
    //chunks_per_period = chunks;

    mutexes mut = {this};

    Streamer::peerSet = peerset_init(0); // CHECK
    network->initializeSignaling(this->streamer->getSocket(), Streamer::peerSet); // CHECK
    psample_parse_data(this->streamer->getPeersampleContext(), NULL, 0);

    // initialize mutexes
    pthread_mutex_init(&this->chunkBufferMutex, NULL);
    pthread_mutex_init(&this->topologyMutex, NULL);
    pthread_mutex_init(&this->peerChunkMutex, NULL);

    // create threads
    pthread_create(&receiveDataThread, NULL, Threads::receiveData, (void *) &mut); // Thread for receiving data
    pthread_create(&sendTopologyThread, NULL, Threads::sendTopology, (void *) &mut); // Thread for sharing the topology of the p2p network
    pthread_create(&generateChunkThread, NULL, Threads::generateChunk, (void *) &mut); // Thread for generating chunks
    pthread_create(&sendChunkThread, NULL, Threads::sendChunk, (void *) &mut); // Thread for sending the chunks

    // join threads
    pthread_join(generateChunkThread, NULL);
    pthread_join(receiveDataThread, NULL);
    pthread_join(sendTopologyThread, NULL);
    pthread_join(sendChunkThread, NULL);
}

void *Threads::receiveData(void *mut) {
#ifdef DEBUG
    fprintf(stdout, "Thread started::receiveData\n");
#endif
    mutexes *m = (mutexes *) mut;
    Threads *t = m->t;

    Network *network = Network::getInstance();

    while (!Threads::stopThreads) {
        int numberOfReceivedBytes;
        nodeID *remote;
        static uint8_t buffer[BUFFSIZE];

        numberOfReceivedBytes = recv_from_peer(t->streamer->getSocket(), &remote, buffer, BUFFSIZE);
        switch (buffer[0] /* Message Type */) {
            case MSG_TYPE_TOPOLOGY:
                pthread_mutex_lock(&t->topologyMutex);
                psample_parse_data(t->streamer->getPeersampleContext(), buffer, BUFFSIZE);
                pthread_mutex_unlock(&t->topologyMutex);
                break;
            case MSG_TYPE_CHUNK:
                fprintf(stderr, "Some dumb peer pushed a chunk to me!\n");
                break;
            case MSG_TYPE_SIGNALLING:
                nodeID *owner;
                int maxDeliver;
                uint16_t transId;
                int chunkToSend;
                signaling_type signalingType;
                int result;
                ChunkIDSet *chunkIDSetReceived;

                result = parseSignaling(buffer + 1, numberOfReceivedBytes - 1, &owner, &chunkIDSetReceived, &maxDeliver, &transId, &signalingType);
                if (owner)
                    nodeid_free(owner);

                switch (signalingType) {
                    case sig_accept:
                        break;
                    case sig_deliver:
                        break;
                    case sig_ack:
                        break;
                    case sig_offer: // Peer offers x chunks...
                        // I AM THE SERVER - I DON'T ACCEPT CHUNKS
                        //fprintf(stdout, "1) Message OFFER: peer offers %d chunks\n", chunkID_set_size(Streamer::chunkIDSet));
                        //chunkToSend = chunkID_set_get_latest(Streamer::chunkIDSet);
                        ////printChunkID_set(Streamer::chunkIDSet);
                        //Streamer::chunkIDSetReceive = chunkID_set_init("size=1");
                        //fprintf(stdout, "2) Acceping only latest chunk #%d\n", chunkToSend);
                        //chunkID_set_add_chunk(Streamer::chunkIDSetReceive, chunkToSend);
                        //acceptChunks(remote, Streamer::chunkIDSetReceive, transId++);
                        break;
                    case sig_request: // peer requests x chunks
                        fprintf(stdout, "1) Message REQUEST: peer requests %d chunks\n", chunkID_set_size(Streamer::chunkIDSet));
                        //printChunkID_set(Streamer::chunkIDSet);
                        chunkToSend = chunkID_set_get_earliest(Streamer::chunkIDSet);

                        pthread_mutex_lock(&t->chunkBufferMutex);
                        pthread_mutex_lock(&t->peerChunkMutex);
                        bool res;
                        res = network->addToPeerChunk(remote, chunkToSend); // add peer/chunk to a list, for sending in sendChunk thread

                        if (res == true) {
                            // we have the chunk, we could send it to the remote peer
                            ChunkIDSet *chunkIDSetRemote = chunkID_set_init("size=1");
                            fprintf(stdout, "2) Deliver only earliest chunk #%d\n", chunkToSend);
                            chunkID_set_add_chunk(chunkIDSetRemote, chunkToSend);
                            deliverChunks(remote, chunkIDSetRemote, transId++); // tell remote that this chunk could be delivered
                        } else {
                            // the requested chunk is too old, send current buffermap
                            ChunkIDSet *chunkIDSetRemote = network->chunkBufferToBufferMap();
                            sendBufferMap(remote, network->getLocalID(), chunkIDSetRemote, Streamer::chunkBufferSize, transId);
                        }
                        pthread_mutex_unlock(&t->peerChunkMutex);
                        pthread_mutex_unlock(&t->chunkBufferMutex);

                        break;
                    case sig_send_buffermap: // peer sent a buffer of chunks
                        // I AM THE SERVER - I DON'T ACCEPT CHUNKS
                        //fprintf(stdout, "1) Message SEND_BMAP: I received a buffer of %d chunks\n", chunkID_set_size(Streamer::chunkIDSet));
                        ////printChunkID_set(Streamer::chunkIDSet);
                        //Streamer::chunkIDSetRemote = chunkID_set_init("size=15,type=bitmap");
                        //if (!Streamer::chunkIDSetRemote) {
                        //    fprintf(stderr, "Unable to allocate memory for Streamer::chunkIDSetRemote\n");
                        //    return -1;
                        //}
                        //fillChunkID_set(Streamer::chunkIDSetRemote, Streamer::random_bmap);
                        //fprintf(stdout, "2) Message SEND_BMAP: I send my buffer of %d chunks\n", chunkID_set_size(Streamer::chunkIDSetRemote));
                        ////printChunkID_set(rcset);
                        //sendBufferMap(remote, t->streamer->getSocket(), Streamer::chunkIDSetRemote, 0, transId++);
                        break;
                    case sig_request_buffermap: // peer requests my buffer map
                        // I AM THE SERVER - I HAVE ALL CHUNKS
                        //fprintf(stdout, "1) Message REQUEST_BMAP: Someone requeste my buffer map [%d]\n", (Streamer::chunkIDSet == NULL));
                        //Streamer::chunkIDSetRemote = chunkID_set_init(sprintf("size=%d,type=bitmap", Streamer::chunkBufferSize));
                        //if (!Streamer::chunkIDSetRemote) {
                        //    fprintf(stderr, "Unable to allocate memory for Streamer::chunkIDSetReceive\n");
                        //    return -1;
                        //}
                        //fillChunkID_set(Streamer::chunkIDSetRemote, random_bmap);
                        //sendBufferMap(remote, t->streamer->getSocket(), Streamer::chunkIDSetRemote, 0, transId++);
                        break;
                }
                break;
            default:
                fprintf(stderr, "Unknown Message Type %x\n", buffer[0]);
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
        psample_parse_data(t->streamer->getPeersampleContext(), NULL, 0);
#ifdef DEBUG
        neighbours = psample_get_cache(t->streamer->getPeersampleContext(), &numberOfNeighbours);
        printf("I have %d neighbours:\n", numberOfNeighbours);
        for (int i = 0; i < numberOfNeighbours; i++) {
            node_addr(neighbours[i], addr, 256);
            printf("\t%d: %s\n", i, addr);
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

    Input *input = Input::getInstance();

    while (!Threads::stopThreads) {
        pthread_mutex_lock(&t->chunkBufferMutex);
        input->generateChunk();
        pthread_mutex_unlock(&t->chunkBufferMutex);

        /*
        const int *fd = input->getInFDs();
        int my_fd[10];
        int i = 0;
        if (fd != NULL) {
            while (fd[i] != -1) {
                my_fd[i] = fd[i];
                i++;
            }
            my_fd[i] = -1;

            wait4data(NULL, NULL, my_fd);
        }
        */
        usleep((useconds_t) input->getPeriod());
    }

    return NULL;
}

void *Threads::sendChunk(void *mut) {
#ifdef DEBUG
    fprintf(stdout, "Thread started::sendChunk\n");
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
