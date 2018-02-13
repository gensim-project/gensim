/*
 * signals.h
 *
 *  Created on: 16 Jan 2015
 *      Author: harry
 */

#ifndef SIGNALS_H_
#define SIGNALS_H_

#include <csignal>
#include <unistd.h>

class System;

bool signals_capture();
void signals_register(System *system);

#endif /* SIGNALS_H_ */
