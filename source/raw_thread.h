// Created by Behnam Dezfouli and Immanuel Amirtharaj
// SIOTLAB
// raw_thread.h


#ifndef _RAW_THREAD_H
#define _RAW_THREAD_H

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


#include "I2C.h"
#include "INA219.h"
#include "Entries.h"

// background thread handler which is responsible for writing raw data to file
void* thread_file_writer(void* arg);

#endif
