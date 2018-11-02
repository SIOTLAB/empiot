// Created by Immanuel Amirtharaj and Behnam Dezfouli
// SIOTLAB
// Entries.h
// Defines the different entries for energy measurment and raw measurment.

#ifndef _ENTRIES_H
#define _ENTRIES_H

enum EntryType {
    E_None = 0,     // Regular entry
    E_Single = 1,   // Used to denote an entry for single annotation when saving to file
    E_Double = 2    // Used to denote an entry for double annotation when saving to file
};

// Data structure to store energy interval data
class energyEntry {
public:
    long long identifier;
    timespec start_time;
    timespec end_time;
    long long energy_consumption_nj;
    float energy_consumption_j;

};

// Data structure to store raw data
class sampleEntry {
public:
    timespec time;
    float shunt_voltage;
    float bus_voltage;
    float current;
    long long identifier;
    EntryType type;
    sampleEntry() {
        type = E_None;
    }
};

#endif
