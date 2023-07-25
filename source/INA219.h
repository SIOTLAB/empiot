// Created by Immanuel Amirtharaj
// SIOTLAB
// INA219.h
// This class is the main driver that sets calibration, reading, and writing for samples.

/*
 
 // Calibration functions - current settings for range, resolution, and voltage
 void setCalibration_16V_400mA_12bits(void);
 void setCalibration_16V_400mA_9bits(void);
 void setCalibration_16V_800mA_12bits(void);
 void setCalibration_16V_800mA_9bits(void);
 
 // To get single readings
 float& get_current_MA();
 Gets the current current in mA by reading the current register
 
 float& get_shunt_voltage_MV ()
 Gets the current shunt voltage in mV by reading the shunt voltage register
 
 float& get_bus_voltage_V()
 Gets the current bus voltage in V by reading the bus voltage register
 
 float& get_bus_voltage_V_set_cvrd()
 Gets the bus voltage and also sets variable "conversion_ready" if the conversion ready bit is set
 
 void status();
 Prints out current, shunt voltage, and bus voltage
 
 int16_t get_conversion_ready_bit();
 Returns the conversion ready bit.  Use this for getting the status of whether the register is ready
 to be read from
 
 void read_power_reg();
 Reads the power value from the power register
 
 
 */

#ifndef INA219_H_
#define INA219_H_

/*
This file contains configuration register values taken from
https://github.com/adafruit/Adafruit_INA219/blob/master/Adafruit_INA219.h
Use these to specify range, resolution, and averaging.
 */

#include <inttypes.h>
#include "I2C.h"
#include <iostream>
#include <inttypes.h>

#define REG_CONFIG  (0x00)
#define REG_SHUNTVOLTAGE (0x01)
#define REG_BUSVOLTAGE (0x02)
#define REG_POWER (0x03)
#define REG_CURRENT (0x04)
#define REG_CALIBRATION (0x05)



#define INA219_CONFIG_RESET                    (0x8000)  // Reset Bit

#define INA219_CONFIG_BVOLTAGERANGE_MASK       (0x2000)  // Bus Voltage Range Mask
#define INA219_CONFIG_BVOLTAGERANGE_16V        (0x0000)  // 0-16V Range
#define INA219_CONFIG_BVOLTAGERANGE_32V        (0x2000)  // 0-32V Range

#define INA219_CONFIG_GAIN_1_40MV              (0x0000)  // Gain 1, 40mV Range
#define INA219_CONFIG_GAIN_2_80MV              (0x0800)  // Gain 2, 80mV Range
#define INA219_CONFIG_GAIN_4_160MV             (0x1000)  // Gain 4, 160mV Range
#define INA219_CONFIG_GAIN_8_320MV             (0x1800)  // Gain 8, 320mV Range

#define INA219_CONFIG_BADCRES_MASK             (0x0780)  // Bus ADC Resolution Mask
#define INA219_CONFIG_BADCRES_9BIT             (0x0080)  // 9-bit bus res = 0..511
#define INA219_CONFIG_BADCRES_10BIT            (0x0100)  // 10-bit bus res = 0..1023
#define INA219_CONFIG_BADCRES_11BIT            (0x0200)  // 11-bit bus res = 0..2047
#define INA219_CONFIG_BADCRES_12BIT            (0x0400)  // 12-bit bus res = 0..4097

#define INA219_CONFIG_SADCRES_MASK             (0x0078)  // Shunt ADC Resolution and Averaging Mask
#define INA219_CONFIG_SADCRES_9BIT_1S_84US     (0x0000)  // 1 x 9-bit shunt sample
#define INA219_CONFIG_SADCRES_10BIT_1S_148US   (0x0008)  // 1 x 10-bit shunt sample
#define INA219_CONFIG_SADCRES_11BIT_1S_276US   (0x0010)  // 1 x 11-bit shunt sample
#define INA219_CONFIG_SADCRES_12BIT_1S_532US   (0x0018)  // 1 x 12-bit shunt sample
#define INA219_CONFIG_SADCRES_12BIT_2S_1060US  (0x0048)	 // 2 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_4S_2130US  (0x0050)  // 4 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_8S_4260US  (0x0058)  // 8 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_16S_8510US (0x0060)  // 16 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_32S_17MS   (0x0068)  // 32 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_64S_34MS   (0x0070)  // 64 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_128S_69MS  (0x0078)  // 128 x 12-bit shunt samples averaged together

#define INA219_CONFIG_MODE_MASK                (0x0007)  // Operating Mode Mask
#define INA219_CONFIG_MODE_POWERDOWN           (0x0000)
#define INA219_CONFIG_MODE_SVOLT_TRIGGERED     (0x0001)
#define INA219_CONFIG_MODE_BVOLT_TRIGGERED     (0x0002)
#define INA219_CONFIG_MODE_SANDBVOLT_TRIGGERED (0x0003)
#define INA219_CONFIG_MODE_ADCOFF              (0x0004)
#define INA219_CONFIG_MODE_SVOLT_CONTINUOUS    (0x0005)
#define INA219_CONFIG_MODE_BVOLT_CONTINUOUS    (0x0006)
#define INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS (0x0007)


class INA219 {

	public:
	INA219();

        void setCalibration_16V_400mA_12bits(void);
        void setCalibration_16V_400mA_9bits(void);
        
        void setCalibration_16V_800mA_12bits(void);
        void setCalibration_16V_800mA_9bits(void);

        void setCalibration_32V_2A_12bit(void);
        void setCalibration_32V_1A_12bit(void);

	float& get_current_MA();
	float& get_shunt_voltage_MV ();
	float& get_bus_voltage_V();
        float& get_bus_voltage_V_set_cvrd(); // Gets the bus voltage and also sets variable "conversion_ready" if the conversion ready bit is set
	void status();
        int16_t get_conversion_ready_bit();
        void read_power_reg();
        bool conversion_ready;
    
	private:
		I2C *connection;
		uint8_t address;
		int busNo;
		uint16_t config;
		uint32_t calValue;

        int16_t current_reg;
        int16_t shunt_voltage_reg;
        int16_t bus_voltage_reg;
        int16_t bus_voltage_reg_shifted;
    
        float currentDivider_mA;
        float powerDivider_mW;

        float current_mA;
        float shunt_voltage_mV;
        float bus_voltage_V;
    
		void get_current_raw();
		void get_shunt_voltage_raw();
		void get_bus_voltage_raw();

        static const uint32_t var_REG_CONFIG = REG_CONFIG;
        static const uint32_t var_REG_SHUNTVOLTAGE = REG_SHUNTVOLTAGE;
        static const uint32_t var_REG_BUSVOLTAGE = REG_BUSVOLTAGE;
        static const uint32_t var_REG_POWER = REG_POWER;
        static const uint32_t var_REG_CURRENT = REG_CURRENT;
        static const uint32_t var_REG_CALIBRATION = REG_CALIBRATION;
};

#endif
