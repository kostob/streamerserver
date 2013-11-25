/*
 * File:   Threads.hpp
 * Author: tobias
 *
 * Created on 18. Oktober 2013, 13:11
 */

#ifndef THREADS_HPP
#define	THREADS_HPP

#define BUFFSIZE 64 * 1024

class Threads {
public:
    Threads();
    Threads(const Threads& orig);
    virtual ~Threads();
    void startThreads(Streamer *s);

    pthread_mutex_t chunkBufferMutex;
    pthread_mutex_t topologyMutex;
    pthread_mutex_t peerChunkMutex;
    static bool stopThreads;
private:
    static void *receiveData(void *mut);
    static void *sendTopology(void *mut);
    static void *generateChunk(void *mut);
    static void *sendChunk(void *mut);

    static int chunks_per_period;
    static int gossipingPeriod;
    static int done;
    static nodeID *s;
    Network *network;
    Streamer *streamer;
};

#endif	/* THREADS_HPP */

