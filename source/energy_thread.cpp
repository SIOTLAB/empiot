// Created by Behnam Dezfouli and Immanuel Amirtharaj
// SIOTLab
// energy_thread.cpp

#ifndef _ENERGY_THREAD_CPP
#define _ENERGY_THREAD_CPP

#include "energy_thread.h"
#include "stop_conditions.h"

using namespace std;

extern vector<energyEntry> ebuffer1, ebuffer2;

// 0 to save data into first buffer, 1 for second buffer
extern int ebuffer_used;

// bool to determine whether to use buffer 1 or buffer 2
extern bool euse_buffer1;

// 1 means flush buffer 1 and 2 means flush buffer 2 
extern volatile int eflush_buffer;

// True if program is reading from the chip
extern bool sampling;

// the current energy entry
extern energyEntry energy_entry;

// File to save data into
extern FILE *dataFile;

extern pthread_mutex_t mutex_file_write_samples;
extern pthread_cond_t start_file_write_samples;

extern bool quit_from_trigger;

void teardown() {

    cout << "\n\nTEARDOWN" << endl;
    cout << "--------" << endl;
    cout << "Finished sampling and writing to the file" << endl;
    fflush(dataFile);
    fclose(dataFile);
}


// Added a wrapper function in the case we want to check something before adding
void write_energy(energyEntry &newEntry) {
    fprintf(dataFile, "%lld\t%lu\t%lu\t%lu\t%lu\t%lld\t%.9f\n", newEntry.identifier, newEntry.start_time.tv_sec, newEntry.start_time.tv_nsec, newEntry.end_time.tv_sec, newEntry.end_time.tv_nsec, newEntry.energy_consumption_nj, newEntry.energy_consumption_j);
}


// Writes the values of the two energy buffers to the file
void* thread_file_writer_energy (void* arg) {
    
    while (1) {

        // waits for energy signal to write to file
        pthread_cond_wait(&start_file_write_samples, &mutex_file_write_samples);

        #ifdef VERBOSE_DEBUG
            cout << "writing energy to file" << endl;
        #endif

        // flushing buffer 1
        if (eflush_buffer == 1) {

            for (std::vector<energyEntry>::iterator it = ebuffer1.begin(); it < ebuffer1.end(); ++it) {

                write_energy(*it);
            }

            fflush(dataFile);
            ebuffer1.clear();

        }

        // flushing buffer 2
        if (eflush_buffer == 2) {

            for (std::vector<energyEntry>::iterator it = ebuffer2.begin(); it < ebuffer2.end(); ++it) {
                write_energy(*it);
            }

            fflush(dataFile);
            ebuffer2.clear();
        }
 
        // when sampling becomes false, we may still have some entries left in the buffer
        if (sampling == false) {

            // flushing buffer 1 
            if (eflush_buffer == 1) {

                for (std::vector<energyEntry>::iterator it = ebuffer1.begin(); it < ebuffer1.end(); ++it) {
                    write_energy(*it);
                }
                fflush(dataFile);
                ebuffer1.clear();
            }

            // flushing buffer 2
            if (eflush_buffer == 2) {

                for (std::vector<energyEntry>::iterator it = ebuffer2.begin(); it < ebuffer2.end(); ++it) {
                    write_energy(*it);
                }

                fflush(dataFile);
                ebuffer2.clear();
            }
        }

        if (quit_from_trigger == true || time_reached() || samples_reached()) {
            break;
        }
    }

    for (std::vector<energyEntry>::iterator it = ebuffer1.begin(); it < ebuffer1.end(); ++it) {
        write_energy(*it);
    }

    for (std::vector<energyEntry>::iterator it = ebuffer2.begin(); it < ebuffer2.end(); ++it) {
        write_energy(*it);
    }

    teardown();

    return NULL;

}

#endif
