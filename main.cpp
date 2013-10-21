/* 
 * File:   main.cpp
 * Author: tobias
 *
 * Created on 16. Oktober 2013, 07:47
 */

#include <iostream>
#include <string>

#include "Streamer.hpp"
using namespace std;

/*
 * 
 */
int main(int argc, char* argv[]) {

    Streamer* s = new Streamer();

    s->parseCommandLineArguments(argc, argv);

    if (s->init() == 0) {
        s->loop();
    }

    return 0;
}
