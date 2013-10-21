/* 
 * File:   Network.hpp
 * Author: tobias
 *
 * Created on 21. Oktober 2013, 12:06
 */

#ifndef NETWORK_HPP
#define	NETWORK_HPP

#include <iostream>
using namespace std;

class Network {
public:
    Network();
    Network(const Network& orig);
    virtual ~Network();
    string createInterface(string interface);
    void updatePeers(struct peerset *peerSet, const uint8_t *buffer, int lenght);
private:

};

#endif	/* NETWORK_HPP */

