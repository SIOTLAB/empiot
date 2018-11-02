// Created by Behnam Dezfouli and Immanuel Amirtharaj
// SIOTLAB
// stop_conditions.h


#ifndef _STOP_CONDITIONS_H
#define _STOP_CONDITIONS_H

#include <sys/resource.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <limits>
#include <fstream>
#include <stdint.h>
#include <sstream>
#include <climits>
#include <errno.h>
#include <string.h>

#include <wiringPi.h>

#include <unistd.h>
#include <pthread.h>

#include <time.h>
#include <sys/time.h>


#include "Entries.h"


bool is_sample_based();
bool is_time_based();
bool quit_trigger();
bool time_reached();
bool samples_reached();

#endif
