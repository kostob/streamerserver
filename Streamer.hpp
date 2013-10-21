/*
 * File:   Streamer.hpp
 * Author: tobias
 *
 * Created on 17. Oktober 2013, 12:19
 */

#ifndef STREAMER_HPP
#define	STREAMER_HPP

#include <iostream>
#include <string>

#include "Network.hpp"

using namespace std;

class Streamer {
public:
    Streamer();
    Streamer(const Streamer& orig);
    virtual ~Streamer();
    void parseCommandLineArguments(int argc, char* argv[]);
    bool init();
    void startThreads();
private:
    // configuration
    string configFilename;
    string configInterface;
    int configPort;

    nodeID socket;
    Network *network;
};

#endif	/* STREAMER_HPP */
