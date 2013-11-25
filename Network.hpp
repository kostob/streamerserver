/*
 * File:   Network.hpp
 * Author: tobias
 *
 * Created on 21. Oktober 2013, 12:06
 */

#ifndef NETWORK_HPP
#define	NETWORK_HPP

#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
    // GRAPES
#include <net_helper.h>
#include <peer.h>
#include <peerset.h>
#include <chunk.h>
#include <scheduler_common.h>
#include <trade_sig_ha.h>
#ifdef __cplusplus
}
#endif
using namespace std;

class Network { // class is a singleton
public:
    static Network *getInstance();
    string createInterface(string interface);
    void sendChunksToPeers();
    int initializeSignaling(nodeID *myID, PeerSet *ps);
    bool addToPeerChunk(nodeID *remote, int chunkId);
    ChunkIDSet *chunkBufferToBufferMap();
    nodeID *getLocalID();

private:
    Network();
    Network(const Network& orig);
    virtual ~Network();

    nodeID *localID;
    PeerSet *peerSet;
    static timeval timeoutBufferMap;
};

#endif	/* NETWORK_HPP */

