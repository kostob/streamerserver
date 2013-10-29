/*
 * File:   Network.hpp
 * Author: tobias
 *
 * Created on 21. Oktober 2013, 12:06
 */

#ifndef NETWORK_HPP
#define	NETWORK_HPP

//Type of signaling message
//Request a ChunkIDSet
#define MSG_SIG_REQ 0
//Diliver a ChunkIDSet (reply to a request)
#define MSG_SIG_DEL 1
//Offer a ChunkIDSet
#define MSG_SIG_OFF 2
//Accept a ChunkIDSet (reply to an offer)
#define MSG_SIG_ACC 3
//Receive the BufferMap
#define MSG_SIG_BMOFF 4
//Request the BufferMap
#define MSG_SIG_BMREQ 5

#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
// GRAPES
#include <net_helper.h>
#include <peer.h>
#include <peerset.h>
#include <chunk.h>
#ifdef __cplusplus
}
#endif
using namespace std;

struct sig_nal {
    uint8_t type; //type of signal.
    uint8_t max_deliver; //Max number of chunks to deliver.
    uint16_t cb_size;
    uint16_t trans_id; //future use...
    uint8_t third_peer; //for buffer map exchange from other peers, just the first byte!
};

class Network { // class is a singleton
public:
    static Network *getInstance();
    string createInterface(string interface);
    void updatePeers(peerset *peerSet, const uint8_t *buffer, int lenght);
    void sendChunkToPeerSet(peerset *peerSet);
    int initializeSignaling(nodeID *myID, peerset *ps);
    void sendBufferMapToPeer(peer *to);
    static int needs(peer *p, chunk *c);
    static int _needs(chunkID_set *cset, int cb_size, int cid);
    static double randomPeer(peer **p);
    static double getChunkTimestamp(chunk **c);

private:
    Network();
    Network(const Network& orig);
    virtual ~Network();
    chunkID_set *chunkBufferToBufferMap();
    int sendSignalling(int type, nodeID *to_id, nodeID *owner_id, chunkID_set *cset, int max_deliver, int cb_size, int trans_id);

    nodeID *localID;
    peerset *peerSet;
    static timeval tout_bmap;
};

#endif	/* NETWORK_HPP */

