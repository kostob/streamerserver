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
    Threads(Network *network);
    Threads(const Loop& orig);
    virtual ~Threads();
    static void *receiveData(void *network);
    static void *sendTopology(void *network);
    static void *forgeChunk(void *network);
    static void *sendChunk(void *network);
private:
    static bool stopThreads = false;
    static int chunks_per_period = 1;
    static int period = 500000;
    static int done;
    static pthread_mutex_t cb_mutex;
    static pthread_mutex_t topology_mutex;
    static struct nodeID *s;
    static struct peerset *peerSet;
    Network *network;
};

#endif	/* THREADS_HPP */

