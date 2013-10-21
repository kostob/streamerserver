/* 
 * File:   Network.cpp
 * Author: tobias
 * 
 * Created on 21. Oktober 2013, 12:06
 */

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

#include "Network.hpp"

Network::Network() {
}

Network::Network(const Network& orig) {
}

Network::~Network() {
}

string Network::createInterface(string interface) {
#ifdef DEBUG
    fprintf(stdout, "Called Streamer::createInterface with parameter interface=%s\n", interface);
#endif

    int socketHandle;
    int res;
    struct ifreq iface_request;
    struct sockaddr_in *sin;
    char buff[512];

    socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (socketHandle < 0) {
        fprintf(stderr, "Could not create socket\n");
        exit(EXIT_FAILURE);
    }

    memset(&iface_request, 0, sizeof (struct ifreq));
    sin = (struct sockaddr_in *) &iface_request.ifr_addr;
    strcpy(iface_request.ifr_name, interface);
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

void Network::updatePeers(struct peerset *peerSet, const uint8_t *buffer, int lenght) {
    int n_ids, i;
    const struct nodeID **ids;
    struct peer *peers;
    struct timeval tnow, told;


    topParseData(buffer, lenght);
    ids = topGetNeighbourhood(&n_ids);
    peerset_add_peers(peerSet, ids, n_ids);

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