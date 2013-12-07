/* 
 * File:   InputFactory.cpp
 * Author: tobias
 * 
 * Created on 7. Dezember 2013, 11:09
 */

#include "InputInterface.hpp"
#include "InputFactory.hpp"
#include "InputFfmpeg.hpp"

InputInterface *InputFactory::createInput(string configInputType) {
    InputInterface *res;

    // FFMPEG
    if (configInputType.compare("ffmpeg") == 0) {
        fprintf(stdout, "Loading input type \"FFMPEG\"\n");
        res = new InputFfmpeg();
    //} else if (configInputType.compare("OTHER") == 0) {
    //    Code for loading other custom input types
    //    fprintf(stdout, "Loading input type \"OTHER\"\n");
    //    res = InputOTHER::getInstance();        
    } else {
        fprintf(stderr, "Error: wrong input type selected\n");
        res = NULL;
    }

    return res;
}

