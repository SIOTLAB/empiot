
// Created by Immanuel Amirtharaj and Behnam Dezfouli
//
// Internet of Things Research Lab
// Santa Clara University, California
// ----------------------
// empiot.cpp

/*
 
 Initialization functions
 These functions are responsible for parsing arguments, initializing GPIO pins.
 
 void append_parser(int cur_pos, int argc, char* argv[]);
 void block_parser(int cur_pos, int argc, char* argv[]);
 void energy_parser(int cur_pos, int argc, char* argv[]);
 void shared_setup();

 
 Sampling functions
 These functions are responsible for starting and stopping measurement.
 In addition, they also are responsible for data aggregation.
 
 void startSampling(void);
 void stopSampling(void);
 void* thread_sampler (void* arg);
 void append_raw(sampleEntry &new_entry);
 
 
 Trigger functions & handlers
 These functions are responsible for registering GPIO triggers, sockets, and their respective
 handlers.
 
 int register_annotation_triggers();
 void* quitHandler(void * arg);
 int sample_on_trigger();
 void* listen_for_comm(void *);
 void handle_message(string message);
 
 
 Printing functions
 
 void printStart();
 void printQuitMessage();
 
 Annotating functions
 These functions are responsible for adding annotations to the text file.
 
 void add_annotation();
 void add_double_annotation();
 
 */

#ifndef _MAIN_TWOBUFFER_CPP
#define _MAIN_TWOBUFFER_CPP

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
#include <bcm2835.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include "INA219.h"
#include "I2C.h"
#include "raw_thread.h"
#include "energy_thread.h"
#include "stop_conditions.h"
#include "Entries.h"
#include <assert.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

using namespace std;

#define OUT_PIN1   RPI_V2_GPIO_P1_13

#define START_TRIGGER 0
#define STOP_TRIGGER 1

#define SINGLE_ANNOTATION_TRIGGER 3    // draw 1 line  PIN 22
#define DOUBLE_ANNOTATION_TRIGGER 4    // draw 2 lines PIN 23

#define BILLION 1000000000L

pthread_mutex_t mutex_start_stop = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start_sampling = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutex_file_write_samples = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start_file_write_samples = PTHREAD_COND_INITIALIZER;


float shunt_resistor = 0.1;

struct timespec start_time;
struct timespec end_time;
struct timespec current_time;
uint64_t diff;

// Need to keep track of running total of samples for -s mode
volatile long int total_samples;

volatile long int interval_samples; // the numebr of samples collected during this sampling interval (start - stop)

volatile bool sampling;

volatile bool run_experiment;

bool append_arg = false;

bool use_buffer1;
bool euse_buffer1;

int buffer_used;
int ebuffer_used;

void startSampling(void);
void stopSampling(void);

// Current shunt, bus, and current values
volatile float shunt = 0;
volatile float bus = 0;
volatile float current = 0;

long long identifier = 0;
long long num_triggers = 0;

// Current energy entry (if in energy collection mode)
energyEntry energy_entry;

// Current and previous entries of raw samples
// We need both of these to calculate energy
sampleEntry current_entry;
sampleEntry previous_entry;

// Depends on memory size
static int buffer_capacity = 30000;

// Buffer to store raw entries
// Only added to in raw entry mode
vector<sampleEntry> buffer1, buffer2;

// Buffer to store energy
// Only added to in energy entry mode
vector<energyEntry> ebuffer1, ebuffer2;


// Both these variables signify which buffer to flush
// 1 = flush buffer 1
// 2 = flush buffer 2
volatile int flush_buffer;
volatile int eflush_buffer;

// This is the result file that saves the information
FILE *dataFile;

// The file name passed in as a parameter
string dataFileName;

// If verbose mode is true, then print the raw data while saving energy to file
bool verbose_mode = false;

// Help determine which setting we are testing for
int num_samples = -1;       
long long time_needed = -1;
int trigger_based = false;
int num_lines = 0;
volatile bool keep_running = true;

// quit from trigger
bool quit_from_trigger = false;

// Energy type
enum EnergyType {
    None = 0,
    V1 = 1,
    V2 = 2
};

// If energy type is None, save raw data to file if not calculate energy at intervals and plot that
EnergyType energy_type = None;
long long total_energy = 0;


struct timespec start_annotation;
struct timespec start_annotation_double;

#define TIME_PIN_1 RPI_V2_GPIO_P1_15
#define TIME_PIN_2 RPI_V2_GPIO_P1_16


// Function declarations
void startSampling(void);
void stopSampling(void);
void append_raw(sampleEntry &new_entry);
void handle_message(string message);
void* listen_for_comm(void *);
void* thread_sampler (void* arg);
int sample_on_trigger();
void add_annotation();
void add_double_annotation();
int register_annotation_triggers();
void* quitHandler(void * arg);
void printStart();
void printQuitMessage();
void shared_setup();
void append_parser(int cur_pos, int argc, char* argv[]);
void block_parser(int cur_pos, int argc, char* argv[]);
void energy_parser(int cur_pos, int argc, char* argv[]);


// Server stuff
#define PORT_NO 5000

// Start sampling when pin START_TRIGGER shows a falling edge (it goes back to high after that)
void startSampling(void) {
    if (sampling == false)
    {
        cout << "\nActivated the trigger to start sampling" << endl;

        printQuitMessage();

        pthread_mutex_lock(&mutex_start_stop);
        
        interval_samples = 0;
        total_energy = 0;


        // reset the identifier for trigger mode ONLY RAW DATA
        if (energy_type == None) {
            identifier = 0;
        }
        
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        // set sampling to be true to tell sampling thread to start sampling        
        sampling = true;

        // send start_sampling trigger to sampling thread
        // Sampling thread has a while loops which waits for this signal
        pthread_cond_signal(&start_sampling);

        // counting the number of triggers
        num_triggers++;

        pthread_mutex_unlock(&mutex_start_stop);
    }
}


// Stop sampling when pin STOP_TRIGGER shows a falling edge (it goes back to high after that)
void stopSampling(void) {
    if (sampling == true)
    {
        
        cout << "Stopping the trigger" << endl;

        // lock because we are updating buffers
        pthread_mutex_lock(&mutex_start_stop);
        
        // tell sampling thread to wait and not sample 
        sampling = false;
        
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        
        diff = BILLION * (end_time.tv_sec - start_time.tv_sec) + end_time.tv_nsec - start_time.tv_nsec;
        printf("elapsed time = %llu nanoseconds\n", (long long unsigned int) diff);

        pthread_mutex_unlock(&mutex_start_stop);


        // If no energy type is specified, we are in raw data collection mode
        // If None, add to raw val to the raw buffer, if not, add energy to the energy buffer
        if (energy_type == None) {
            if ( use_buffer1 == true )
            {
                use_buffer1 == false; // if the sampling is resumed immediatelt, then we need to write to another buffer as the file write buffer is using this buffer
                flush_buffer = 1;
                buffer_used = 0;
                pthread_cond_signal(&start_file_write_samples);
            }
            else
            {
                use_buffer1 == true; // if the sampling is resumed immediately, then we need to write to another buffer as the file write buffer is using this buffer
                flush_buffer = 2;
                buffer_used = 0;
                pthread_cond_signal(&start_file_write_samples);
            }
        }

        // Do not need to do this for energy, because we want to keep on buffering values
    }
}


void append_raw(sampleEntry &new_entry) {

    // buffer_used keeps track of the current size of the buffer
    buffer_used++;

    // Adding to the buffer if in raw collection mode
    if (energy_type == None) {
        if ((use_buffer1 == true) && (buffer_used < buffer_capacity))
        {
            buffer1.push_back(new_entry);
        }
        else if ((use_buffer1 == true) && (buffer_used == buffer_capacity))
        {
            buffer1.push_back(new_entry); // This is the last entry added to buffer 1
            flush_buffer = 1; // The file write thread will flush buffer 1
            use_buffer1 = false;
            if (buffer2.size() != 0)
                cout << "Buffer 2 not empty! " << endl;
            pthread_cond_signal(&start_file_write_samples);
            buffer_used = 0;
        }
        else if ((use_buffer1 == false) && (buffer_used < buffer_capacity))
        {
            buffer2.push_back(new_entry);
        }
        else
        {
            buffer2.push_back(new_entry);
            flush_buffer = 2;
            use_buffer1 = true;
            if (buffer1.size() != 0)
                cout << "Buffer 1 not empty!" << endl;
            pthread_cond_signal(&start_file_write_samples);
            buffer_used = 0;
        }
    }
}

void handle_message(string message) {

    if (message == "start") {
        startSampling();
    }
    else if (message == "stop") {
        stopSampling();
    }
    else {
        cout << "Received an invalid command" << endl;
    }
}

void* listen_for_comm(void *) {

    // create a socklen_t struct so we can pass its length into the recvfrom function
    socklen_t len;
    int port = PORT_NO;
    
    
    // create the socket
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    
    // create and configure a sockaddr_in struct which specifies the local/remote endpoint address to which to connect the socket to
    struct sockaddr_in server;
    bzero((char *)&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;    

    // Attempts to bind the socket and returns an integer which signifies whether binding was successful or not
    int bind_return = bind(s, (struct sockaddr*)& server, sizeof(server));
 
    if (bind_return != 0) {
        cout << "Binding failed, exiting program" << endl;
        return NULL;
    }
    else {
        cout << "Server listening on port:  " << PORT_NO << endl;
    }

    while (true) {
        char res[100];
        // At this point, program begins to listen to the client.  Returns an integer to determine whether it was able to receive anything from the client
        int check = recvfrom(s, res, 100, 0, (struct sockaddr *)&server, &len);
        if (check < 0) {
            cerr << "Error in receiving" << endl;
        }
        else {
         //   cout << check << endl;
            handle_message(string(res, check-1));
        }
    }
    return NULL;
}

// This thread samples the sensed values of the INA219 chip
void* thread_sampler (void* arg) {

    INA219 *driver = (INA219*)arg;
    if (sampling == false) {
        // wait for the trigger to send signal to resume sampling
        pthread_cond_wait(&start_sampling, &mutex_start_stop);

    }

    pthread_mutex_unlock(&mutex_start_stop);

    while (1)
    {
        while (sampling == true)
        {
            // stop conditions which determine when sampling finishes
            if (samples_reached() || time_reached() || quit_trigger()) {
                keep_running == false;
                sampling = false;
                continue;
            }

            // In order to get a valid sample, we have to poll the conversion ready bit, which notifies that a 
            // valid sample is available
            do {
                bus = driver -> get_bus_voltage_V_set_cvrd();
            } while(driver -> conversion_ready == false);
            
            // get raw information
            shunt = driver->get_shunt_voltage_MV();
            // bus = driver->get_bus_voltage_V();
            //current = driver->get_current_MA();
            current = shunt / shunt_resistor ;
            
            // get current time for current entry timestamp
            clock_gettime(CLOCK_MONOTONIC, &current_time);
            
            
            // Addition to the queue should be an atomic function
            // We need this because the stop thread will notify the file write thread based on the values set below - we need to make sure the values are consistent
            pthread_mutex_lock(&mutex_start_stop);
            
            // increment total_samples for -s mode
            total_samples++;
            
            // increment interval_samples
            interval_samples++;
            
            // set previous entry
            previous_entry = current_entry;

            // populate current entry with information
            current_entry.time = current_time;
            current_entry.shunt_voltage = shunt;
            current_entry.bus_voltage = bus;
            current_entry.current = current;
            current_entry.identifier = identifier;

            
            if (shunt < 0 || bus < 0 || current < 0) {
                cerr << "Shunt, bus, or current < 0, this is wrong" << endl;
                cerr << "shunt(mV) :: "  << shunt << " bus(V) :: " << bus << endl;
                continue;
            }


            // If there is no 
            if (energy_type == None) {
                identifier++;
            }

            // For verbose mode we just print everything on the screen (terminal)
            if (verbose_mode == true) {
                cout << "Bus Voltage (V): " << current_entry.bus_voltage << "  Shunt Voltage (mV): " << current_entry.shunt_voltage << "  Current (mA): " << current_entry.current << endl;
            }
            
            append_raw(current_entry);

            pthread_mutex_unlock(&mutex_start_stop);


            // when energy computation is enabled, calculate the energy
            if (energy_type != None && interval_samples > 1) {
		
                // power = V * I (Done in volts) where bus_voltage is in V and current in mA
                float current_power = (current_entry.bus_voltage) * (current_entry.current * 0.001);
                float previous_power = (previous_entry.bus_voltage) * (previous_entry.current * 0.001);
                
                // changing from unsigned to signed
                // Take the area to convert energy
                long long time_diff = BILLION * (current_entry.time.tv_sec - previous_entry.time.tv_sec) + (current_entry.time.tv_nsec - previous_entry.time.tv_nsec);
                
                // calculate the whole rectangle (V1)
                long long interval_energy = time_diff * current_power;

                if (time_diff < 0) {
                    cerr << time_diff << endl;
                }

                assert(time_diff >= 0);

                
                if (energy_type == V2) {
                    // shave off the triangle (if V2)
                    long long shaved_area = ((current_power - previous_power) * time_diff) / 2;
                    interval_energy -= shaved_area;
                }
                
                // increment to running total of energy
                total_energy += interval_energy;
            	//cout << "Interval ENERGY: " << time_diff << ", " << interval_energy<<", "<< total_energy<< "..............................." << endl;
            }
        }

        int diffy = (current_entry.time.tv_sec - start_time.tv_sec);



        // Done sampling in the interval.  Now, we can add the energy
        cout << "\nFINISHED SAMPLING INTERVAL " << num_triggers << endl;
        cout << "----------------------------" << endl;

        cout << "Total number Samples:    " << total_samples << endl;
        cout << "Interval Samples:    " << interval_samples << endl;
        // cout << "Time Taken:    " << endl;
        printf("Time Taken: %d seconds\n",  diffy);
        printQuitMessage();

        pthread_mutex_lock(&mutex_start_stop);

        // Add an energy entry to the energy buffer
        if (energy_type != None) {

            energy_entry.start_time = start_time;
            energy_entry.end_time = current_time;
            energy_entry.energy_consumption_nj = total_energy;
            energy_entry.energy_consumption_j = (float) (total_energy / (float) 1000000000.0);
            energy_entry.identifier = identifier;
 
            cout << "Interval Energy: " << energy_entry.energy_consumption_j  << " joules" << endl;


            // increment identifier (using this to ensure data saved in order)
            identifier++;
                  
            // need to know how many entries the current buffer has (increment because adding)
            ebuffer_used++;
                    
            //  Add entries to the double buffer
            if ((euse_buffer1 == true) && (ebuffer_used < buffer_capacity)) {

                ebuffer1.push_back(energy_entry);
            }
            else if (euse_buffer1 == true && ebuffer_used >= buffer_capacity) {

                cout << "Buffer 1 filled" << endl;
                ebuffer1.push_back(energy_entry);
                eflush_buffer = 1;
                euse_buffer1 = false;
                if (ebuffer2.size() != 0) {
                    cout << "Buffer 2 not empty " << endl;
                }
                cout << "Starting to write to file " << endl;
                pthread_cond_signal(&start_file_write_samples);
                ebuffer_used = 0;

            }
            else if ((euse_buffer1 == false) && (ebuffer_used < buffer_capacity)) {
                ebuffer2.push_back(energy_entry);
            }
            else {
                cout << "Buffer 2 filled" << endl;
                ebuffer2.push_back(energy_entry);
                eflush_buffer = 2;
                euse_buffer1 = true;
                if(ebuffer1.size() != 0) {
                    cout << "Buffer 1 not empty" << endl;
                }

                pthread_cond_signal(&start_file_write_samples);
                ebuffer_used = 0;
            }

            // Done calculating energy consumption for the interval, reset it back to 0
            total_energy = 0;
        }
        
        // We need to return from the thread_sampler when the time is up or the number of required samples has been collected
        if (is_time_based() || is_sample_based() || quit_trigger()) {
            
            //quit_from_trigger = true;

            // unlock first because we are returning out of this function
            pthread_mutex_unlock(&mutex_start_stop);

            // tell file writer thread to write samples
            pthread_cond_signal(&start_file_write_samples);

            return NULL;
        }
        
        // We should wait for a trigger to restart the smapling
        else {

            // At this point, waiting for trigger to resume sampling
            while (sampling == false) {
                // wait for the trigger to send signal to resume sampling
                pthread_cond_wait(&start_sampling, &mutex_start_stop);
            }
        }

        
        pthread_mutex_unlock(&mutex_start_stop);

        if (quit_from_trigger == true) {
            cout << "Exiting trigger" << endl;
            pthread_cond_signal(&start_file_write_samples);
            return NULL;
        }
    }
    return NULL;
}



int sample_on_trigger() {

    cout << "Setting up sampling on trigger" << endl;
    
    sampling = false;
    if (wiringPiSetup () < 0) {
        fprintf (stderr, "Unable to setup wiringPi: %s\n", strerror (errno));
        assert(wiringPiSetup() < 0);

        return -1;
    }
    
    // and attach myInterrupt() to the interrupt
    if ( wiringPiISR (START_TRIGGER, INT_EDGE_FALLING, &startSampling) < 0 ) {
        fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno));
        return -1;
    }
    
    if ( wiringPiISR (STOP_TRIGGER, INT_EDGE_FALLING, &stopSampling) < 0 ) {
        fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno));
        return -1;
    }
    
    pullUpDnControl(START_TRIGGER, PUD_UP); // We need to enable the pull-up resistor for this input pin
    pullUpDnControl(STOP_TRIGGER, PUD_UP); // We need to enable the pull-up resistor for this input pin
    
    return 1;
}



void add_annotation() {

    cout << "Interrupt received - Single" << endl;

    struct timespec tmp;
    clock_gettime(CLOCK_MONOTONIC, &tmp);

    long long time_diff = BILLION * (tmp.tv_sec - start_annotation.tv_sec) + (tmp.tv_nsec - start_annotation.tv_nsec);

    if (time_diff < 2000000) {
        cout << "Time difference is lesser than 2ms, let us skip this interrupt" << endl;
        return;
    }

    start_annotation = tmp;

    pthread_mutex_lock(&mutex_start_stop);

    sampleEntry placeholder;
    placeholder.time = tmp;
    placeholder.type = E_Single;
    append_raw(placeholder);

    pthread_mutex_unlock(&mutex_start_stop);
}

void add_double_annotation() {

    cout << "Interrupt received - Single" << endl;

    struct timespec tmp;
    clock_gettime(CLOCK_MONOTONIC, &tmp);

    long long time_diff = BILLION * (tmp.tv_sec - start_annotation_double.tv_sec) + (tmp.tv_nsec - start_annotation_double.tv_nsec);

    if (time_diff < 2000000) {
        cout << "Time difference is lesser than 1ms, let us skip this interrupt" << endl;
        return;
    }

    start_annotation_double = tmp;

    pthread_mutex_lock(&mutex_start_stop);

    sampleEntry placeholder;
    placeholder.time = tmp;
    placeholder.type = E_Double;
    append_raw(placeholder);

    pthread_mutex_unlock(&mutex_start_stop);
}

int register_annotation_triggers() {

    cout << "Registering annotation triggers" << endl;

    if (wiringPiSetup() < 0) {
        fprintf (stderr, "Unable to setup wiringPi: %s\n", strerror (errno));
        assert(wiringPiSetup() < 0);
    }

     // and attach myInterrupt() to the interrupt PIN 26
    if ( wiringPiISR (SINGLE_ANNOTATION_TRIGGER, INT_EDGE_FALLING, &add_annotation) < 0 ) {
        fprintf (stderr, "Unable to setup ISR - annotation triggers: %s\n", strerror (errno));
        return -1;
    }

    if ( wiringPiISR (DOUBLE_ANNOTATION_TRIGGER, INT_EDGE_FALLING, &add_double_annotation) < 0 ) {
        fprintf (stderr, "Unable to setup ISR - annotation triggers: %s\n", strerror (errno));
        return -1;
    }

    pullUpDnControl(SINGLE_ANNOTATION_TRIGGER, PUD_UP); // We need to enable the pull-up resistor for this input pin
    pullUpDnControl(DOUBLE_ANNOTATION_TRIGGER, PUD_UP); // We need to enable the pull-up resistor for this input pin

    return 0;
    
}

// callback to a background thread which listens for user input 'q'
// quits the program
void* quitHandler(void * arg) {
    char cmd[10];
    while(1) {
        char cmd[10];
        scanf("%s", cmd);

        if (string(cmd) == "q") {
            cout << "Quitting the program" << endl;

            // set quit global variable to true
            quit_from_trigger = true;
            sampling = true;
           // pthread_cond_signal(&start_file_write_samples);            
            pthread_cond_signal(&start_sampling);


            cout << "The number of triggers is " << num_triggers << endl;
            cout << "The number of samples read is " << total_samples << endl;

            return NULL;
        }
        else {
            cout << "Invalid command, enter 'q' to quit" << endl;
        }
    }
}

void printStart() {
    cout << "\n\nSTARTING ENERGY MEASUREMENT" << endl;
    cout << "---------------------------" << endl;
    printQuitMessage();
}

void printQuitMessage() {
    cout << "\nEnter q to quit (not Ctrl^C)" << endl;
}


void shared_setup() {
    void* exitStatus;
    pthread_t thread_sampler_id, thread_file_writer_id, thread_file_writer_energy_id, thread_quitter_id, thread_listener_id;
    
    cout << "\n\nSETTING UP EMPIOT" << endl;
    cout << "-----------------" << endl;

    keep_running = true;
    
    
    use_buffer1 = true; // initially we use buffer1
    buffer_used = 0; // initially the number of used buffer entries is 0

    euse_buffer1 = true;
    ebuffer_used = 0;
    
    total_samples = 0;
    interval_samples = 0;

    cout << "Sample Enty Size:  " << sizeof(sampleEntry) << endl;

    
    cout << "Buffer Capacity Size:  " << buffer_capacity << endl;
    buffer1.reserve(buffer_capacity);
    buffer2.reserve(buffer_capacity);

    ebuffer1.reserve(buffer_capacity);
    ebuffer2.reserve(buffer_capacity);
    
    if (append_arg == false) {
        cout << "Created a new file called " << dataFileName << endl;
        dataFile = fopen(dataFileName.c_str(), "w");
        
        if (energy_type == V1 || energy_type == V2)
        {
            fprintf(dataFile, "1: identifier, 2: start_time.tv_sec, 3: start_time.tv_nsec, 4: end_time.tv_sec, 5: end_time.tv_nsec, 6: energy_consumption_nj, 7: energy_consumption_j\n");
        }
        else
            fprintf(dataFile, "1: identifier, 2: time.tv_sec, 3: time.tv_nsec , 4: shunt_voltage (mV), 5: bus_voltage (V), current (mA)\n");

    } else {
        cout << "Appending to an existing file; name: " << dataFileName << endl;
        dataFile = fopen(dataFileName.c_str(), "a");
    }


    INA219 *driver = new INA219();
    
    // Calibration...
    cout << "Calibrating...!" << endl;
    
    // See INA219.h for more calibration options
    if (true)
    {
        driver->setCalibration_32V_1A_12bit();
        // driver->setCalibration_16V_800mA_12bits();
        // driver->setCalibration_16V_400mA_12bits();
        
        cout << "Calibration type: 16V_800mA_12bits" << endl;    
    }
    cout << "Calibrating...Done!" << endl;
    
    
    int which = PRIO_PROCESS;
    id_t pid;
    int prio;
    int max_prio = NZERO - 1;
    
    pid = getpid();
    prio = getpriority(which, pid);
    cout << "Process priority: " << prio << endl;
    prio = setpriority(which, pid, max_prio);
    prio = getpriority(which, pid);
    cout << "New process priority: " << prio << endl;
    
    
    // Setting thread priority
    pthread_attr_t tattr_sampling_thread, tattr_sample_writing_thread, tattr_energy_writing_thread, tattr_quitter_thread, tattr_listener_thread;
    pthread_t tid;
    int ret;
    int prio_sampling_thread = 99; // using priority 1 the buffers overflow
    int prio_sample_write_thread = 99; // using priority 1 the buffers overflow
    int prio_energy_write_thread = 99; // using priority 1 the buffers overflow
    
    sched_param param;
    
    /* initialized with default attributes */
    ret = pthread_attr_init (&tattr_sampling_thread);
    /* safe to get existing scheduling param */
    ret = pthread_attr_getschedparam (&tattr_sampling_thread, &param);
    /* set the priority; others are unchanged */
    param.sched_priority = prio_sampling_thread;
    /* setting the new scheduling param */
    ret = pthread_attr_setschedparam (&tattr_sampling_thread, &param);

    ret = pthread_attr_init(&tattr_quitter_thread);
    ret = pthread_attr_init(&tattr_listener_thread);

    
    // The thread which manages sampling
    pthread_create(&thread_sampler_id, &tattr_sampling_thread, thread_sampler, driver);

    // The thread which manages the quitting from terminal
    pthread_create(&thread_quitter_id, &tattr_quitter_thread, quitHandler, NULL);

    // The thread which manages the listening
    pthread_create(&thread_listener_id, &tattr_listener_thread, listen_for_comm, NULL);
    
    /* initialized with default attributes */
    ret = pthread_attr_init (&tattr_sample_writing_thread);
    /* safe to get existing scheduling param */
    ret = pthread_attr_getschedparam (&tattr_sample_writing_thread, &param);
    /* set the priority; others are unchanged */
    param.sched_priority = prio_sample_write_thread;
    /* setting the new scheduling param */
    ret = pthread_attr_setschedparam (&tattr_sample_writing_thread, &param);
    
    
    /* initialized with default attributes */
    ret = pthread_attr_init (&tattr_energy_writing_thread);
    /* safe to get existing scheduling param */
    ret = pthread_attr_getschedparam (&tattr_energy_writing_thread, &param);
    /* set the priority; others are unchanged */
    param.sched_priority = prio_energy_write_thread;
    /* setting the new scheduling param */
    ret = pthread_attr_setschedparam (&tattr_energy_writing_thread, &param);
    
    if (energy_type == None) {
        cout << "Energy Storing Mode:   Raw Data" << endl;

        // Only register annotations when we take raw samples
        register_annotation_triggers();

        printStart();

        // The thread which manages writing raw samples to the file
        pthread_create(&thread_file_writer_id, &tattr_sample_writing_thread, thread_file_writer, NULL);
        
        pthread_join(thread_file_writer_id, &exitStatus);
    }
    else
    {
        cout << "Energy Storing Mode:   Calculated Energy Measurement" << endl;
        
        printStart();

        //thread which manages writing energy samples to the file
        pthread_create(&thread_file_writer_id, &tattr_sample_writing_thread, thread_file_writer_energy, NULL);
        
        pthread_join(thread_file_writer_id, &exitStatus);
    }

    pthread_join(thread_sampler_id, &exitStatus);
}

void append_parser(int cur_pos, int argc, char* argv[]) {
    if (cur_pos < argc && string(argv[cur_pos]) == "-a") {
       // cout << "Appending to existing file" << endl;
        append_arg = true;
    }
}

// function to set the buffer size (number of entries in each buffer)
// Defaults to 512
void block_parser(int cur_pos, int argc, char* argv[]) {
    int block_arg = cur_pos;
    int block_size_arg = cur_pos + 1;

    if (block_arg < argc && string(argv[block_arg]) == "-b") {
        if (block_size_arg < argc) {
            buffer_capacity = atoi(argv[block_size_arg]);
            append_parser(block_size_arg+1, argc, argv);
        }
    }
    else {
        append_parser(block_arg, argc, argv);
    }
    
}

// parses out the energy requirements
void energy_parser(int cur_pos, int argc, char* argv[]) {
    int energy_arg = cur_pos;
    int energy_type_pos = energy_arg + 1;

    if (energy_arg < argc && string(argv[energy_arg]) == "-e") {
        if (energy_type_pos < argc) {
            if (string(argv[energy_type_pos]) == "v1") {
                energy_type = V1;
            }
            else {
                energy_type = V2;
            }
            block_parser(cur_pos + 2, argc, argv);
        }
    }
    else {
        block_parser(cur_pos, argc, argv);
    }
}



int main(int argc, char* argv[]) {
    
    int args_needed = 3;
    
    int verbose_arg = 1;
    int file_arg = 2;
    int type_arg = 3;
    int duration_arg = 4;
    int energy_arg = -1;
    int energy_type_pos = -1;
    
    sampling = false;


    // initialize the start annotation
    clock_gettime(CLOCK_MONOTONIC, &start_annotation);


    if (argc < args_needed) {
        cout << "number of arguments is not specified" << endl;
        return 0;
    }
    
    if (string(argv[verbose_arg]) == "-v") {
        verbose_mode = true;
    }
    else {
        file_arg--;
        type_arg--;
        duration_arg--;
        energy_arg--;
        energy_type_pos--;
    }
    
    dataFileName = string(argv[file_arg]);
    string type  = string(argv[type_arg]);
    
    // trigger sampling
    if (type == "-g") {
        // Do energy parsing first
        energy_parser(type_arg + 1, argc, argv);

        int success = sample_on_trigger();
        shared_setup();
        return 0;
    }
    
    // time sampling
    else if (type == "-t") {
        if (argc < args_needed + 1) {
            cout << "Need to enter amount of time in seconds" << endl;
            return 0;
        }
        
        time_needed = atoi(argv[duration_arg]);
        
        energy_parser(duration_arg + 1, argc, argv);
        
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        
        // since measuring time, set sampling to be true
        sampling = true;
        
        shared_setup();
    }
    
    // Num samples sampling
    else if (type == "-s") {
        if (argc < args_needed + 1) {
            cout << "Need to enter the number of samples" << endl;
            return 0;
        }
        
        num_samples = atoi(argv[duration_arg]);
        
        // Do energy parsing start_file_write_samples
        energy_parser(duration_arg + 1, argc, argv);

        sampling = true;
        
        shared_setup();
        startSampling();
    }
    else if (type == "-r") {
        cout << "Just reading and printing" << endl;
        cout << "Press Ctrl ^C to exit" << endl;
        
        INA219 driver = INA219();
        while(1) {
            driver.status();
            sleep(1);
        }
    }
    else {
        cout << "Invalid arguments" << endl;
    }

    return 0;
}

#endif
