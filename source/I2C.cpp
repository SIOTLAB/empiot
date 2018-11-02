// Created by Immanuel Amirtharaj
// SIOTLAB
// I2C.cpp

#include "I2C.h"

using namespace std;

//#define LINUX_I2C_DEV 1
//#define WIRING_I2C_DEV 1
#define BROADCOM_I2C_DEV 1


I2C::I2C(int bus, int address) {
	_i2cbus = bus;
	_i2caddr = address;
	snprintf(busfile, sizeof(busfile), "/dev/i2c-%d", bus);

	#ifdef LINUX_I2C_DEV
		openfd();
	#elif WIRING_I2C_DEV
		wfd = wiringPiI2CSetup(0x40);
	#else
		bcm2835_init();
		bcm2835_i2c_begin();
		bcm2835_i2c_setSlaveAddress(0x40);
		bcm2835_i2c_set_baudrate(2000000);
	#endif
}


I2C::~I2C() {
	#ifdef LINUX_I2C_DEV
		openfd();
	#elif BROADCOM_I2C_DEV
		bcm2835_close();
	#endif
}


uint16_t I2C::read_register(const uint8_t& reg) {
    // Broadcom library uses char instead of int
	#ifdef BROADCOM_I2C_DEV
		char data[3];
	#else
		uint8_t data[3];
	#endif

	data[0] = reg;

	#ifdef LINUX_I2C_DEV
		if (write(fd, data, 1) != 1) {
			perror("pca9555ReadRegisterPair set register");
		}
		if (read(fd, data, 2) != 2) {
			perror("pca9555ReadRegisterPair read value");
		}
	#elif WIRING_I2C_DEV
		wiringPiI2CWrite(wfd, data[0]);
		data[0] = wiringPiI2CRead(wfd);
		data[1] = wiringPiI2CRead(wfd);

	#else
		//cerr << "Using BCM2" << endl;
		bcm2835_i2c_write(data, 1);
		bcm2835_i2c_read(data, 2);
	#endif

	uint16_t res = data[1] | (data[0] << 8);
	return res;

}
 
void I2C::write_register(uint8_t reg, uint16_t value) {
    // Broadcom library uses char instead of int
	#ifdef BROADCOM_I2C_DEV
	char data[3];
	#else
	uint8_t data[3];
	#endif

	data[0] = reg;
	data[1] = (value >> 8) & 0xff;
	data[2] = value & 0xff;
	
	#ifdef LINUX_I2C_DEV
		if (write(fd, data, 3) != 3) {
			perror("pca9555SetRegisterPair");
		}
	#elif WIRING_I2C_DEV
		wiringPiI2CWrite(wfd, data[0]);
		wiringPiI2CWrite(wfd, data[1]);
		wiringPiI2CWrite(wfd, data[2]);
	#else
		//cerr << "Writing this data " << data << endl;
		bcm2835_i2c_write(data, 3);
	#endif

}
// Open device file for I2C Device
void I2C::openfd() {
	if ((fd = open(busfile, O_RDWR)) < 0) {
		syslog(LOG_ERR, "Couldn't open I2C Bus %d [openfd():open %d]", _i2cbus,
				errno);
	}
	if (ioctl(fd, I2C_SLAVE, _i2caddr) < 0) {
		syslog(LOG_ERR, "I2C slave %d failed [openfd():ioctl %d]", _i2caddr,
				errno);
	}
}
