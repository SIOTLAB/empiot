

#include "stop_conditions.h"

using namespace std;

extern int total_samples;

extern int num_samples;
extern int time_needed;
extern bool quit_from_trigger;

extern struct timespec current_time;
extern struct timespec start_time;


bool is_sample_based() {
    return num_samples > 0;
}

bool is_time_based() {
    return time_needed > 0;
}

bool quit_trigger() {

    return quit_from_trigger;
}

// When the sampling time duration is finished
bool time_reached() {
    
    long long unsigned elapsed = (current_time.tv_sec  - start_time.tv_sec);
    long long unsigned reached = time_needed;
    
    return (time_needed > -1) && (elapsed >= reached);
}

// When the required number of samples has been collected
bool samples_reached() {
    return num_samples > -1 && total_samples >= num_samples;
}
