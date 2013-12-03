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

// GRAPES
#ifdef __cplusplus
extern "C" {
#endif
#include <net_helper.h>
#include <peer.h>
#include <chunkidset.h>
#include <tman.h>
#include <peerset.h>
#include <chunk.h>
#include <scheduler_ha.h>
#include <grapes_msg_types.h>
#include <trade_msg_ha.h>
#include <trade_sig_la.h>
#ifdef __cplusplus
}
#endif    

#include "Streamer.hpp"
#include "Network.hpp"

using namespace std;

timeval Network::timeoutBufferMap = {3, 0};

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

bool Network::addToPeerChunk(nodeID* remote, int chunkId) {
    // check if chunk is not too old and is still in the chunk buffer
    if (cb_get_chunk(Streamer::chunkBuffer, chunkId) != NULL) {
        Streamer::peerChunks = (PeerChunk*) realloc(Streamer::peerChunks, sizeof (PeerChunk));
        peer *p;
        p = (peer*) malloc(sizeof (peer));
        char ip[256];
        node_ip(remote, ip, 256);
        p->id = create_node(ip, node_port(remote));
        Streamer::peerChunks[Streamer::peerChunksSize].peer = p;
        Streamer::peerChunks[Streamer::peerChunksSize].chunk = chunkId;
        ++Streamer::peerChunksSize;
    } else {
        return false;
    }
    return true;
}

void Network::sendChunksToPeers() {
    for (int i = 0; i < Streamer::peerChunksSize; ++i) {
        const chunk *c = cb_get_chunk(Streamer::chunkBuffer, Streamer::peerChunks[i].chunk);

        if (c != NULL && Streamer::peerChunks[i].peer != NULL) { // TODO: check if peer is still in psampler
#ifdef DEBUG
            char addressRemote[256];
            node_addr(Streamer::peerChunks[i].peer->id, addressRemote, 256);
            fprintf(stdout, "DEBUG: Sending chunk %d to peer %s\n", Streamer::peerChunks[i].chunk, addressRemote);
#endif
            sendChunk(Streamer::peerChunks[i].peer->id, c, 0);
            free(Streamer::peerChunks[i].peer->id);
            Streamer::peerChunks[i].peer->id = NULL;
        } else {
#ifdef DEBUG
            //char addressRemote[256];
            //node_addr(Streamer::peerChunks[i].peer->id, addressRemote, 256);
            fprintf(stdout, "DEBUG: Sending chunk %d failed: chunk to old (not in chunkbuffer anymore)\n", Streamer::peerChunks[i].chunk);
#endif
        }
        free(Streamer::peerChunks[i].peer);
        Streamer::peerChunks[i].peer = NULL;
    }

    // reset peerChunks
    free(Streamer::peerChunks);
    Streamer::peerChunks = NULL;
    Streamer::peerChunksSize = 0;
}

ChunkIDSet *Network::chunkBufferToBufferMap() {
    struct chunk *chunks;
    int num_chunks, i;
    chunkID_set *my_bmap = chunkID_set_init(0);
    chunks = cb_get_chunks(Streamer::chunkBuffer, &num_chunks);

    for (i = 0; i < num_chunks; i++) {
        chunkID_set_add_chunk(my_bmap, chunks[i].id);
    }
    return my_bmap;
}

void Network::offerChunksToPeers() {
    int maximumNumberOfOffers = 10;
    int numberOfPeers;
    const nodeID * const* peers;
    peers = psample_get_cache(Streamer::peersampleContext, &numberOfPeers);

    for (int i = 0; i < numberOfPeers && i < maximumNumberOfOffers; ++i) {
        offerChunks((nodeID*) peers[i], Streamer::chunkIDSet, 50, 0);
    }
}
