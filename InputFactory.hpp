/* 
 * File:   InputFactory.hpp
 * Author: tobias
 *
 * Created on 7. Dezember 2013, 11:09
 */

#ifndef INPUTFACTORY_HPP
#define	INPUTFACTORY_HPP

#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
};
#endif

#include "InputInterface.hpp"

class InputFactory {
public:
    static InputInterface *createInput(string configInputType);
};

#endif	/* INPUTFACTORY_HPP */

