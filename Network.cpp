/*
 * File:   Network.cpp
 * Author: tobias
 *
 * Created on 21. Oktober 2013, 12:06
 */

#include <iostream>
#include <string>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

#ifdef __cplusplus
extern "C" {
#endif
// GRAPES
#include <net_helper.h>
#include <peer.h>
#include <chunkidset.h>
#include <topmanager.h>
#include <peerset.h>
#include <chunk.h>
#include <scheduler_la.h>
#include <grapes_msg_types.h>
#include <trade_msg_ha.h>
#include <trade_sig_la.h>
#ifdef __cplusplus
}
#endif    

#include "Streamer.hpp"
#include "Network.hpp"

using namespace std;

timeval Network::tout_bmap = {3, 0};

Network* Network::getInstance() {
    static Network n;
    return &n;
}

Network::Network() {
}

Network::Network(const Network& orig) {
}

Network::~Network() {
}

string Network::createInterface(string interface) {
#ifdef DEBUG
    fprintf(stdout, "Called Streamer::createInterface with parameter interface=%s\n", interface.c_str());
#endif

    int socketHandle;
    int res;
    ifreq iface_request;
    sockaddr_in *sin;
    char buff[512];

    socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (socketHandle < 0) {
        fprintf(stderr, "Could not create socket\n");
        exit(EXIT_FAILURE);
    }

    memset(&iface_request, 0, sizeof (struct ifreq));
    sin = (sockaddr_in *) & iface_request.ifr_addr;
    strcpy(iface_request.ifr_name, interface.c_str());
    /* sin->sin_family = AF_INET); */
    res = ioctl(socketHandle, SIOCGIFADDR, &iface_request);
    if (res < 0) {
        perror("ioctl(SIOCGIFADDR)");
        close(socketHandle);

        exit(EXIT_FAILURE);
    }
    close(socketHandle);

    inet_ntop(AF_INET, &sin->sin_addr, buff, sizeof (buff));

    return strdup(buff);
}

void Network::updatePeers(peerset *peerSet, const uint8_t *buffer, int lenght) {
    int n_ids, i;
    const nodeID **ids;
    peer *peers;
    timeval tnow, told;


    topParseData(buffer, lenght);
    ids = topGetNeighbourhood(&n_ids);
    peerset_add_peers(peerSet, (nodeID**) ids, n_ids);

    gettimeofday(&tnow, NULL);
    timersub(&tnow, &tout_bmap, &told);
    peers = peerset_get_peers(peerSet);
    for (i = 0; i < peerset_size(peerSet); i++) {
        if (timerisset(&peers[i].bmap_timestamp) && timercmp(&peers[i].bmap_timestamp, &told, <)) {
            topRemoveNeighbour(peers[i].id);
            peerset_remove_peer(peerSet, peers[i--].id);
        }
    }
}

int Network::initializeSignaling(nodeID* myID, peerset* ps) {
    this->localID = myID;
    this->peerSet = ps;
    return 1;
}

void Network::sendChunkToPeerSet(peerset *peerSet) {
    chunk *buff;
    int size, res, n;
    peer *neighbours;

    n = peerset_size(peerSet);
    neighbours = peerset_get_peers(peerSet);

#ifdef DEBUG
    fprintf(stdout, "Send Chunk: %d neighbours\n", n);
#endif

    if (n == 0) return;
    buff = cb_get_chunks(Streamer::cb, &size);

#ifdef DEBUG
    fprintf(stdout, "\t %d chunks in buffer...\n", size);
#endif

    if (size == 0) return;

    /************ STUPID DUMB SCHEDULING ****************/
    int target = n * (rand() / (RAND_MAX + 1.0)); /*0..n-1*/
    int c = size * (rand() / (RAND_MAX + 1.0)); /*0..size-1*/
    /************ /STUPID DUMB SCHEDULING ****************/

#ifdef DEBUG
    fprintf(stdout, "\t sending chunk[%d] (%d) to ", buff[c].id, c);
    fprintf(stdout, "%s\n", node_addr(neighbours[target].id));
#endif

    res = sendChunk(neighbours[target].id, buff + c);
#ifdef DEBUG
    fprintf(stdout, "Result: %d\n", res);
#endif

    // extended scheduler...
    //    int i;
    //    /************ USE SCHEDULER ****************/
    //    size_t selectedpairs_len = 1;
    //    chunk * chunkPeerSet[size];
    //    //peer * peers[n];
    //    nodeID * peers[n];
    //    PeerChunk selectedpairs[1];
    //
    //    for (i = 0; i < size; i++) {
    //        chunkPeerSet[i] = buff + i;
    //    }
    //    for (i = 0; i < n; i++) {
    //        peers[i] = (neighbours + i)->id;
    //    }
    //    schedSelectPeerFirst(SCHED_WEIGHTED, peers, n, chunkPeerSet, size, selectedpairs, &selectedpairs_len, Network::needs, Network::randomPeer, Network::getChunkTimestamp);
    //    //schedSelectPushList(peers, n, chunkPeerSet, size, selectedpairs, &selectedpairs_len);
    //    /************ /USE SCHEDULER ****************/
    //
    //    for (i = 0; i < selectedpairs_len; i++) {
    //        peer *p = selectedpairs[i].peer;
    //        chunk *c = selectedpairs[i].chunk;
    //#ifdef DEBUG
    //        fprintf(stdout, "\t sending chunk[%d] to ", c->id);
    //        fprintf(stdout, "%s\n", node_addr(p->id));
    //#endif
    //
    //        this->sendBufferMapToPeer(p);
    //        res = sendChunk(p->id, c);
    //#ifdef DEBUG
    //        fprintf(stdout, "\tResult: %d\n", res);
    //#endif
    //        if (res >= 0) {
    //            chunkID_set_add_chunk(p->bmap, c->id); //don't send twice ... assuming that it will actually arrive
    //        }
    //    }
}

int Network::sendSignalling(int type, nodeID *to_id, nodeID *owner_id, chunkID_set *cset, int max_deliver, int cb_size, int trans_id) {
    int buff_len, meta_len, msg_len, ret;
    uint8_t *buff;
    struct sig_nal *sigmex;
    uint8_t *meta;

    meta = (uint8_t*) malloc(1024);

    sigmex = (struct sig_nal*) meta;
    sigmex->type = type;
    sigmex->max_deliver = max_deliver;
    sigmex->cb_size = cb_size;
    sigmex->trans_id = trans_id;
    meta_len = sizeof (*sigmex) - 1;
    sigmex->third_peer = 0;
    if (owner_id) {
        meta_len += nodeid_dump(&sigmex->third_peer, owner_id, 1024);
    }

    buff_len = 1 + chunkID_set_size(cset) * 4 + 12 + meta_len; // this should be enough
    buff = (uint8_t*) malloc(buff_len);
    if (!buff) {
        fprintf(stderr, "Error allocating buffer\n");
        return -1;
    }

    buff[0] = MSG_TYPE_SIGNALLING;
    msg_len = 1 + encodeChunkSignaling(cset, meta, meta_len, buff + 1, buff_len - 1);
    free(meta);
    if (msg_len < 0) {
        fprintf(stderr, "Error in encoding chunk set for sending a buffermap\n");
        ret = -1;
    } else {
        send_to_peer(localID, to_id, buff, msg_len);
    }
    ret = 1;
    free(buff);
    return ret;
}

void Network::sendBufferMapToPeer(peer *to) {
    struct chunkID_set *my_bmap = this->chunkBufferToBufferMap();
    //this->sendMyBufferMap(to->id, my_bmap, this->chunkBufferSize, 0);

    this->sendSignalling(MSG_SIG_BMOFF, to->id, this->localID, my_bmap, 0, Streamer::chunkBufferSize, 0);

    chunkID_set_clear(my_bmap, 0);
    free(my_bmap);
}

chunkID_set *Network::chunkBufferToBufferMap() {
    struct chunk *chunks;
    int num_chunks, i;
    struct chunkID_set *my_bmap = chunkID_set_init(0);
    chunks = cb_get_chunks(Streamer::cb, &num_chunks);

    for (i = 0; i < num_chunks; i++) {
        chunkID_set_add_chunk(my_bmap, chunks[i].id);
    }
    return my_bmap;
}

/**
 *example function to filter chunks based on whether a given peer needs them.
 *
 * Looks at buffermap information received about the given peer.
 */
int Network::needs(peer *p, chunk *c) {
#ifdef DEBUG
    fprintf(stdout, "\t%s needs c%d ? :", node_addr(p->id), c->id);
#endif
    if (!p->bmap) {
#ifdef DEBUG
        fprintf(stdout, "no bmap\n");
#endif
        return 1; // if we have no bmap information, we assume it needs the chunk (aggressive behaviour!)
    }
    return _needs(p->bmap, p->cb_size, c->id);
}

int Network::_needs(chunkID_set *cset, int cb_size, int cid) {
    if (chunkID_set_check(cset, cid) < 0) { //it might need the chunk
        int missing, min;
        //@TODO: add some bmap_timestamp based logic

        if (chunkID_set_size(cset) == 0) {
#ifdef DEBUG
            fprintf(stdout, "bmap empty\n");
#endif
            return 1; // if the bmap seems empty, it needs the chunk
        }
        missing = cb_size - chunkID_set_size(cset);
        missing = missing < 0 ? 0 : missing;
        min = chunkID_set_get_chunk(cset, 0);
#ifdef DEBUG
        fprintf(stdout, "%s ... cid(%d) >= min(%d) - missing(%d) ?\n", (cid >= min - missing) ? "YES" : "NO", cid, min, missing);
#endif
        return (cid >= min - missing);
    }
#ifdef DEBUG
    fprintf(stdout, "has it\n");
#endif
    return 0;
}

double Network::randomPeer(peer **p) {
    return 1;
}

double Network::getChunkTimestamp(chunk **c) {
    return (double) (*c)->timestamp;
}
