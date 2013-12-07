/* 
 * File:   InputInterface.hpp
 * Author: tobias
 *
 * Created on 7. Dezember 2013, 10:32
 */

#ifndef INPUTINTERFACE_HPP
#define	INPUTINTERFACE_HPP

#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
    
// GRAPES
#include <net_helper.h>

#ifdef __cplusplus
}
#endif

using namespace std;

class InputInterface {
public:
    virtual bool open(string filename) = 0;
    virtual void close() = 0;
    virtual bool generateChunk() = 0;
    virtual uint8_t getEncryptionDataChunk(nodeID *remote, int chunkId) = 0;
    virtual uint8_t getEncryptionDataLogin(nodeID *remote) = 0;
    virtual bool secureDataEnabledChunk() = 0;
    virtual bool secureDataEnabledLogin() = 0;
    virtual useconds_t getPauseBetweenChunks() = 0;
};

#endif	/* INPUTINTERFACE_HPP */
