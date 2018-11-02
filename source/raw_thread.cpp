// Created by Behnam Dezfouli and Immanuel Amirtharaj
// SIOTLAB
// raw_thread.cpp


#ifndef _RAW_THREAD_CPP
#define _RAW_THREAD_CPP


// Uncomment this to save the timestamp of when the entry was saved to file
//#define SAVE_FILEWRITE_TIME


#include "raw_thread.h"
#include "energy_thread.h"
#include "stop_conditions.h"


using namespace std;

extern vector<sampleEntry> buffer1, buffer2;

extern volatile int flush_buffer;
extern int num_lines;
extern volatile bool sampling;


extern pthread_mutex_t mutex_file_write_samples;
extern pthread_cond_t start_file_write_samples;

extern bool quit_from_trigger;

extern FILE *dataFile;


struct timespec filewrite_time;


string divider = "-----\n";
string divider2 = "*****\n";

void write_raw(sampleEntry &newEntry) {

    if (newEntry.type == E_None) {
        #ifdef SAVE_FILEWRITE_TIME
            clock_gettime(CLOCK_MONOTONIC, &filewrite_time);
            fprintf(dataFile, "%lld\t%lu\t%lu\t%f\t%f\t%f\t%lu\t%lu\n", newEntry.identifier, newEntry.time.tv_sec, newEntry.time.tv_nsec , newEntry.shunt_voltage, newEntry.bus_voltage, newEntry.current, filewrite_time.tv_sec, filewrite_time.tv_nsec);
        #else
            fprintf(dataFile, "%lld\t%lu\t%lu\t%f\t%f\t%f\n", newEntry.identifier, newEntry.time.tv_sec, newEntry.time.tv_nsec , newEntry.shunt_voltage, newEntry.bus_voltage, newEntry.current);
        #endif
    } 
    else if (newEntry.type == E_Single){
        fprintf(dataFile, "1stsubtrigger\t%lu\t%lu\t%s", newEntry.time.tv_sec, newEntry.time.tv_nsec, divider.c_str());
    }   
    else if (newEntry.type == E_Double) {
        fprintf(dataFile, "2ndsubtrigger\t%lu\t%lu\t%s", newEntry.time.tv_sec, newEntry.time.tv_nsec, divider2.c_str());
    }
}


// Writes the values of the two sampling buffers to file
void* thread_file_writer (void* arg) {
    
    while (1) {
        
        #ifdef VERBOSE_DEBUG
            cout << "Entering thread file writer" << " buffer 1 is " << buffer1.size() << " and buffer 2 is " << buffer2.size() << endl;
        #endif
        
        // Waiting until a buffer fills up and then save to file
        pthread_cond_wait(&start_file_write_samples, &mutex_file_write_samples);
        
        // If we should write the data of buffer1 to the disk
        if (flush_buffer == 1) {

            for (std::vector<sampleEntry>::iterator it = buffer1.begin(); it != buffer1.end(); ++it) {      

                write_raw(*it);
                num_lines++;
            }
            
            buffer1.clear();
        }
        
        // If we should write the data of buffer2 to the disk
        if (flush_buffer == 2) {

            for (std::vector<sampleEntry>::iterator it = buffer2.begin(); it != buffer2.end(); ++it) {
                
                write_raw(*it);
                num_lines++;
            }
            
            buffer2.clear();
        }
        
        // What if the sampling is stopped while the file writing is in progress
        // in this case we don't receive any signal from the other thread
        // so we need to check it manually if it is time to flush
        if (sampling == false) {
            #ifdef VERBOSE_DEBUG
                cout << "flushing" << endl;
            #endif

            if (buffer1.size() != 0) {

                for (std::vector<sampleEntry>::iterator it = buffer1.begin(); it != buffer1.end(); ++it) {
                    write_raw(*it);
                    num_lines++;
                }
                
                buffer1.clear();

            }
            
            if (buffer2.size() != 0) {

                for (std::vector<sampleEntry>::iterator it = buffer2.begin(); it != buffer2.end(); ++it) {
                    write_raw(*it);
                    num_lines++;
                }

                buffer2.clear();
            }
        }

        // Fixes bug where only parts of the last entry is written to file
        if (quit_from_trigger == true || time_reached() || samples_reached()) {

            break;
        }
    }

    // In the case there are leftover values in the buffer when the filewriter is stopped, flush them.
    for (std::vector<sampleEntry>::iterator it = buffer1.begin(); it != buffer1.end(); ++it) {
        write_raw(*it);
        num_lines++;
    }

    for (std::vector<sampleEntry>::iterator it = buffer2.begin(); it != buffer2.end(); ++it) {
        write_raw(*it);
        num_lines++;
    }

    teardown();
    return NULL;
}


#endif
