// Created by Immanuel Amirtharaj
// SIOTLAB
// I2C.h
// This class provides functions for communicating via the I2C bus.  It contains functions for opening, reading, and writing registers.

// The following resources were used.
// http://www.airspayce.com/mikem/bcm2835/group__i2c.html#gaae82c3f4b7dfb4dcb51358d1ae878029
// http://www.airspayce.com/mikem/bcm2835/group__i2c.html#gaae82c3f4b7dfb4dcb51358d1ae878029a
// https://www.waveshare.com/wiki/Raspberry_Pi_Tutorial_Series:_PCF8591_AD/DA
// http://www.iot-programmer.com/index.php/books/22-raspberry-pi-and-the-iot-in-c/chapters-raspberry-pi-and-the-iot-in-c/61-raspberry-pi-and-the-iot-in-c-i2c-bus?showall=&start=1
// http://www.iot-programmer.com/index.php/books/22-raspberry-pi-and-the-iot-in-c/chapters-raspberry-pi-and-the-iot-in-c/61-raspberry-pi-and-the-iot-in-c-i2c-bus?showall=&start=2


/*

I2C(int bus, int address)
bus - register 

void write_register(uint8_t reg, uint16_t value);
INPUT
 	- reg - register address
 	- value - value to write to the register
OUTPUT
	- new instance of I2C
 
uint16_t read_register(const uint8_t& reg);
INPUT
	- reg - register address to read from
OUTPUT
	- 16 bit representation of the register value

*/


#ifndef I2C_H_
#define I2C_H_

#include <inttypes.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>     
#include <fcntl.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <syslog.h>	

#include <string.h>
#include <string>
#include <sstream>
#include <iostream>

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <bcm2835.h>


#define BUFFER_SIZE 0x02  // Expecting to read 2 bytes

class I2C {
	public:
		I2C(int, int);
		virtual ~I2C();
		uint16_t dataBuffer[BUFFER_SIZE];
		uint16_t read_register(const uint8_t&);
		void write_register(uint8_t, uint16_t);
    
	private:
		int _i2caddr;
		int _i2cbus;
		void openfd();
		char busfile[64];
		int fd;
		int wfd;
};

#endif 
