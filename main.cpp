/*
 * File:   main.cpp
 * Author: tobias
 *
 * Created on 16. Oktober 2013, 07:47
 */

#include <iostream>
#include <string>

#include "Streamer.hpp"
#include "Threads.hpp"
using namespace std;

/*
 *
 */
int main(int argc, char* argv[]) {

    Streamer* s = new Streamer();
    Threads* t = new Threads();

    s->parseCommandLineArguments(argc, argv);

    if (s->init()) {
        t->startThreads(s);
    }

    return 0;
}
