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

/**
 * Input interface
 * This is the interface for the input modules.
 * For example implementation see InputFfmpeg
 * 
 * @param filename
 * @return 
 */
class InputInterface {
public:
    /**
     * Initialize the Input module
     * 
     * Will try to open the given file/input for streaming
     * 
     * @param filename the path to to the file that will be used for streaming (ex. normal file or /dev/...)
     * @return bool
     */
    virtual bool open(string filename) = 0;
    
    /**
     * Close the input and free space
     * 
     */
    virtual void close() = 0;
    
    /**
     * Generate Chunk
     * This method will generate a chunk from the opened file/input.
     * Watermark or extra calculations have to be done here.
     * Chunks have to be added to Streamer::chunkBuffer and Streamer::chunkIdSet
     * 
     * @return bool
     */
    virtual bool generateChunk() = 0;
    
    /**
     * Get secured data for chunk
     * This method return the secured data for the given chunk and peer
     * 
     * @param remote nodeID of the peer
     * @param chunkId chunk ID
     * @return uint8_t
     */
    virtual uint8_t *getSecuredDataChunk(nodeID *remote, int chunkId) = 0;
    
    /**
     * Get secure data for secure data for beginning of transaction
     * This method will return the secure data for a peer after it connected to
     * the network.
     * 
     * @param remote noteID of the peer
     * @return uint8_t
     */
    virtual uint8_t *getSecuredDataLogin(nodeID *remote) = 0;
    
    /**
     * Check if secure data for chunks are enabled
     * Returns true if enabled, false if not enabled.
     * 
     * @return bool
     */
    virtual bool securedDataEnabledChunk() = 0;
    
    /**
     * Check if secure data for beginning of transaction are enabled
     * Returns true if enabled, false if not enabled
     * 
     * @return bool
     */
    virtual bool securedDataEnabledLogin() = 0;
    
    /**
     * Get the pause between generation of chunks
     * This method will return the time of the pause between generation of chunks.
     * 
     * @return useconds_t
     */
    virtual useconds_t getPauseBetweenChunks() = 0;
};

#endif	/* INPUTINTERFACE_HPP */
