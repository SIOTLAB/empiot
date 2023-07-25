// Created by Immanuel Amirtharaj and Behnam Dezfouli
// September 2017
// SIOTLAB


#include "INA219.h"

using namespace std;


// http://cdwilson.us/articles/understanding-the-INA219/
INA219::INA219() {

	address = 0x40;
	busNo = 1;
    connection = new I2C(busNo, address);
}

void INA219::status() {
    
	float shunt = get_shunt_voltage_MV();
	float bus = get_bus_voltage_V();
	float current = get_current_MA();

	cout << "Shunt: " << shunt << "mV\n" << "Bus: " << bus << "V\n" << "Current: " << current << "mA" << endl;
	cout << endl;
}

void INA219::setCalibration_32V_1A_12bit() {

    //Reset the board first
    uint16_t config = INA219_CONFIG_RESET;
    connection->write_register(var_REG_CONFIG, config);

  // By default we use a pretty huge range for the input voltage,
  // which probably isn't the most appropriate choice for system
  // that don't use a lot of power.  But all of the calculations
  // are shown below if you want to change the settings.  You will
  // also need to change any relevant register settings, such as
  // setting the VBUS_MAX to 16V instead of 32V, etc.

  // VBUS_MAX = 32V		(Assumes 32V, can also be set to 16V)
  // VSHUNT_MAX = 0.32	(Assumes Gain 8, 320mV, can also be 0.16, 0.08, 0.04)
  // RSHUNT = 0.1			(Resistor value in ohms)

  // 1. Determine max possible current
  // MaxPossible_I = VSHUNT_MAX / RSHUNT
  // MaxPossible_I = 3.2A

  // 2. Determine max expected current
  // MaxExpected_I = 1.0A

  // 3. Calculate possible range of LSBs (Min = 15-bit, Max = 12-bit)
  // MinimumLSB = MaxExpected_I/32767
  // MinimumLSB = 0.0000305             (30.5uA per bit)
  // MaximumLSB = MaxExpected_I/4096
  // MaximumLSB = 0.000244              (244uA per bit)

  // 4. Choose an LSB between the min and max values
  //    (Preferrably a roundish number close to MinLSB)
  // CurrentLSB = 0.0000400 (40uA per bit)

  // 5. Compute the calibration register
  // Cal = trunc (0.04096 / (Current_LSB * RSHUNT))
  // Cal = 10240 (0x2800)

  calValue = 10240;

  // 6. Calculate the power LSB
  // PowerLSB = 20 * CurrentLSB
  // PowerLSB = 0.0008 (800uW per bit)

  // 7. Compute the maximum current and shunt voltage values before overflow
  //
  // Max_Current = Current_LSB * 32767
  // Max_Current = 1.31068A before overflow
  //
  // If Max_Current > Max_Possible_I then
  //    Max_Current_Before_Overflow = MaxPossible_I
  // Else
  //    Max_Current_Before_Overflow = Max_Current
  // End If
  //
  // ... In this case, we're good though since Max_Current is less than
  // MaxPossible_I
  //
  // Max_ShuntVoltage = Max_Current_Before_Overflow * RSHUNT
  // Max_ShuntVoltage = 0.131068V
  //
  // If Max_ShuntVoltage >= VSHUNT_MAX
  //    Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Else
  //    Max_ShuntVoltage_Before_Overflow = Max_ShuntVoltage
  // End If

  // 8. Compute the Maximum Power
  // MaximumPower = Max_Current_Before_Overflow * VBUS_MAX
  // MaximumPower = 1.31068 * 32V
  // MaximumPower = 41.94176W

  // Set multipliers to convert raw current/power values
  currentDivider_mA = 25;    // Current LSB = 40uA per bit (1000/40 = 25)
  powerDivider_mW = 0.8f; // Power LSB = 800uW per bit

    // Set Calibration register to 'Cal' calculated above
    //cout << "Writing to calibration register...!" << endl;
    connection->write_register(var_REG_CALIBRATION, calValue);
    //cout << "Done!" << endl;
    
    // Set Config register to take into account the settings above
    config = INA219_CONFIG_BVOLTAGERANGE_32V |
    INA219_CONFIG_GAIN_8_320MV |
    INA219_CONFIG_BADCRES_12BIT |
    INA219_CONFIG_SADCRES_12BIT_1S_532US |
    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
    
    connection->write_register(var_REG_CONFIG, config);
}


void INA219::setCalibration_32V_2A_12bit(void) {

//Reset the board first
uint16_t config = INA219_CONFIG_RESET;
connection->write_register(var_REG_CONFIG, config);

  // By default we use a pretty huge range for the input voltage,
  // which probably isn't the most appropriate choice for system
  // that don't use a lot of power.  But all of the calculations
  // are shown below if you want to change the settings.  You will
  // also need to change any relevant register settings, such as
  // setting the VBUS_MAX to 16V instead of 32V, etc.

  // VBUS_MAX = 32V             (Assumes 32V, can also be set to 16V)
  // VSHUNT_MAX = 0.32          (Assumes Gain 8, 320mV, can also be 0.16, 0.08,
  // 0.04) RSHUNT = 0.1               (Resistor value in ohms)

  // 1. Determine max possible current
  // MaxPossible_I = VSHUNT_MAX / RSHUNT
  // MaxPossible_I = 3.2A

  // 2. Determine max expected current
  // MaxExpected_I = 2.0A

  // 3. Calculate possible range of LSBs (Min = 15-bit, Max = 12-bit)
  // MinimumLSB = MaxExpected_I/32767
  // MinimumLSB = 0.000061              (61uA per bit)
  // MaximumLSB = MaxExpected_I/4096
  // MaximumLSB = 0,000488              (488uA per bit)

  // 4. Choose an LSB between the min and max values
  //    (Preferrably a roundish number close to MinLSB)
  // CurrentLSB = 0.0001 (100uA per bit)

  // 5. Compute the calibration register
  // Cal = trunc (0.04096 / (Current_LSB * RSHUNT))
  // Cal = 4096 (0x1000)

  calValue = 4096;

  // 6. Calculate the power LSB
  // PowerLSB = 20 * CurrentLSB
  // PowerLSB = 0.002 (2mW per bit)

  // 7. Compute the maximum current and shunt voltage values before overflow
  //
  // Max_Current = Current_LSB * 32767
  // Max_Current = 3.2767A before overflow
  //
  // If Max_Current > Max_Possible_I then
  //    Max_Current_Before_Overflow = MaxPossible_I
  // Else
  //    Max_Current_Before_Overflow = Max_Current
  // End If
  //
  // Max_ShuntVoltage = Max_Current_Before_Overflow * RSHUNT
  // Max_ShuntVoltage = 0.32V
  //
  // If Max_ShuntVoltage >= VSHUNT_MAX
  //    Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Else
  //    Max_ShuntVoltage_Before_Overflow = Max_ShuntVoltage
  // End If

  // 8. Compute the Maximum Power
  // MaximumPower = Max_Current_Before_Overflow * VBUS_MAX
  // MaximumPower = 3.2 * 32V
  // MaximumPower = 102.4W

  // Set multipliers to convert raw current/power values
  currentDivider_mA = 10; // Current LSB = 100uA per bit (1000/100 = 10)
  powerDivider_mW = 2; // Power LSB = 1mW per bit (2/1)

    // Set Calibration register to 'Cal' calculated above
    //cout << "Writing to calibration register...!" << endl;
    connection->write_register(var_REG_CALIBRATION, calValue);
    //cout << "Done!" << endl;
    
    
    // Set Config register to take into account the settings above
    config = INA219_CONFIG_BVOLTAGERANGE_32V |
    INA219_CONFIG_GAIN_8_320MV |
    INA219_CONFIG_BADCRES_12BIT |
    INA219_CONFIG_SADCRES_12BIT_1S_532US |
    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
    
    connection->write_register(var_REG_CONFIG, config);
}


void INA219::setCalibration_16V_400mA_12bits(void) {
    
    //Reset the board first
    uint16_t config = INA219_CONFIG_RESET;
    connection->write_register(var_REG_CONFIG, config);
    
    // Calibration which uses the highest precision for
    // current measurement (0.1mA), at the expense of
    // only supporting 16V at 400mA max.
    
    // VBUS_MAX = 16V
    // VSHUNT_MAX = 0.04          (Assumes Gain 1, 40mV)
    // RSHUNT = 0.1               (Resistor value in ohms)
    
    // 1. Determine max possible current
    // MaxPossible_I = VSHUNT_MAX / RSHUNT
    // MaxPossible_I = 0.4A
    
    // 2. Determine max expected current
    // MaxExpected_I = 0.4A
    
    // 3. Calculate possible range of LSBs (Min = 15-bit, Max = 12-bit)
    // MinimumLSB = MaxExpected_I/32767
    // MinimumLSB = 0.0000122              (12uA per bit)
    // MaximumLSB = MaxExpected_I/4096
    // MaximumLSB = 0.0000977              (98uA per bit)
    
    // 4. Choose an LSB between the min and max values
    //    (Preferrably a roundish number close to MinLSB)
    // CurrentLSB = 0.00005 (50uA per bit)
    
    // 5. Compute the calibration register
    // Cal = trunc (0.04096 / (Current_LSB * RSHUNT))
    // Cal = 8192 (0x2000)
    
    calValue = 8192;
    
    //calValue = 33573; // this is my slected value to get the result close to that of DMM Keysight 34465A
    
    // 6. Calculate the power LSB
    // PowerLSB = 20 * CurrentLSB
    // PowerLSB = 0.001 (1mW per bit)
    
    // 7. Compute the maximum current and shunt voltage values before overflow
    //
    // Max_Current = Current_LSB * 32767
    // Max_Current = 1.63835A before overflow
    //
    // If Max_Current > Max_Possible_I then
    //    Max_Current_Before_Overflow = MaxPossible_I
    // Else
    //    Max_Current_Before_Overflow = Max_Current
    // End If
    //
    // Max_Current_Before_Overflow = MaxPossible_I
    // Max_Current_Before_Overflow = 0.4
    //
    // Max_ShuntVoltage = Max_Current_Before_Overflow * RSHUNT
    // Max_ShuntVoltage = 0.04V
    //
    // If Max_ShuntVoltage >= VSHUNT_MAX
    //    Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
    // Else
    //    Max_ShuntVoltage_Before_Overflow = Max_ShuntVoltage
    // End If
    //
    // Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
    // Max_ShuntVoltage_Before_Overflow = 0.04V
    
    // 8. Compute the Maximum Power
    // MaximumPower = Max_Current_Before_Overflow * VBUS_MAX
    // MaximumPower = 0.4 * 16V
    // MaximumPower = 6.4W
    
    // Set multipliers to convert raw current/power values
    currentDivider_mA = 20; //81.96721311;  // Current LSB = 50uA per bit (1000/50 = 20)
    powerDivider_mW = 1;     // Power LSB = 1mW per bit
    
    // Set Calibration register to 'Cal' calculated above
    //cout << "Writing to calibration register...!" << endl;
    connection->write_register(var_REG_CALIBRATION, calValue);
    //cout << "Done!" << endl;
    
    
    // Set Config register to take into account the settings above
    config = INA219_CONFIG_BVOLTAGERANGE_16V |
    INA219_CONFIG_GAIN_1_40MV |
    INA219_CONFIG_BADCRES_12BIT |
    INA219_CONFIG_SADCRES_12BIT_1S_532US |
    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
    
    connection->write_register(var_REG_CONFIG, config);
}


void INA219::setCalibration_16V_400mA_9bits(void) {
    
    //Reset the board first
    uint16_t config = INA219_CONFIG_RESET;
    connection->write_register(var_REG_CONFIG, config);
    
    config = INA219_CONFIG_BVOLTAGERANGE_16V |
    INA219_CONFIG_GAIN_1_40MV |
    INA219_CONFIG_BADCRES_9BIT |
    INA219_CONFIG_SADCRES_9BIT_1S_84US |
    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
    
    connection->write_register(var_REG_CONFIG, config);
}


void INA219::setCalibration_16V_800mA_12bits(void) {
    
    //Reset the board first
    uint16_t config = INA219_CONFIG_RESET;
    connection->write_register(var_REG_CONFIG, config);
    
    
    // Calibration which uses the highest precision for
    // current measurement (0.1mA), at the expense of
    // only supporting 16V at 600mA max.
    
    // VBUS_MAX = 16V
    // VSHUNT_MAX = 0.08         (Assumes Gain 1, 80mV)
    // RSHUNT = 0.1               (Resistor value in ohms)
    
    // 1. Determine max possible current
    // MaxPossible_I = VSHUNT_MAX / RSHUNT
    // MaxPossible_I = 0.8A
    
    // 2. Determine max expected current
    // MaxExpected_I = 0.8A
    
    // 3. Calculate possible range of LSBs (Min = 15-bit, Max = 12-bit)
    // MinimumLSB = MaxExpected_I/32767
    // MinimumLSB = 0.00002441            (24uA per bit)
    // MaximumLSB = MaxExpected_I/4096
    // MaximumLSB = 0.0001953             (190uA per bit)
    
    // 4. Choose an LSB between the min and max values
    //    (Preferrably a roundish number close to MinLSB)
    // CurrentLSB = 0.00005 (50uA per bit)
    
    // 5. Compute the calibration register
    // Cal = trunc (0.04096 / (Current_LSB * RSHUNT))
    // Cal = 8192 (0x2000)
    
    
    //calValue = 8192;
    
    calValue = 8500; // this is our slected value to get the result close to that of DMM Keysight 34465A
    
    
    // Set multipliers to convert raw current/power values
    currentDivider_mA = 20;  // Current LSB = 60uA per bit (1000/50 = 20)
    powerDivider_mW = 1;     // Power LSB = 1mW per bit
    
    // Set Calibration register to 'Cal' calculated above
    //cout << "Writing to calibration register...!" << endl;
    connection->write_register(var_REG_CALIBRATION, calValue);
    //cout << "Done!" << endl;
    
    
    // Set Config register to take into account the settings above
    config = INA219_CONFIG_BVOLTAGERANGE_16V |
    INA219_CONFIG_GAIN_2_80MV |
    INA219_CONFIG_BADCRES_12BIT |
    INA219_CONFIG_SADCRES_12BIT_1S_532US |
    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
    
    connection->write_register(var_REG_CONFIG, config);
}


void INA219::setCalibration_16V_800mA_9bits(void) {
    
    //Reset the board first
    uint16_t config = INA219_CONFIG_RESET;
    connection->write_register(REG_CONFIG, config);
    
    config = INA219_CONFIG_BVOLTAGERANGE_16V |
    INA219_CONFIG_GAIN_2_80MV |
    INA219_CONFIG_BADCRES_9BIT |
    INA219_CONFIG_SADCRES_9BIT_1S_84US |
    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
    
    connection->write_register(var_REG_CONFIG, config);
}



void INA219::get_current_raw() {
	current_reg = connection->read_register(var_REG_CURRENT);
	return;
}


void INA219::get_shunt_voltage_raw() {
	shunt_voltage_reg = connection->read_register(var_REG_SHUNTVOLTAGE);
	return;
}


void INA219::get_bus_voltage_raw() {
	bus_voltage_reg = connection->read_register(var_REG_BUSVOLTAGE);
	bus_voltage_reg_shifted = (bus_voltage_reg >> 3);
	return;
}

int16_t INA219::get_conversion_ready_bit()
{
    return (connection->read_register(var_REG_BUSVOLTAGE));
}

float& INA219::get_current_MA() {
    get_current_raw();
	current_mA = current_reg / currentDivider_mA;  //ina219_currentDivider_mA;
	return current_mA;
}


float& INA219::get_shunt_voltage_MV () {
    get_shunt_voltage_raw();
    shunt_voltage_mV = shunt_voltage_reg * 0.01;
	return shunt_voltage_mV;
}


float& INA219::get_bus_voltage_V() {
	get_bus_voltage_raw();
    bus_voltage_V = bus_voltage_reg_shifted * 0.004 ; //0.0039072; //one LSB of bus voltage is 4 mV
	return bus_voltage_V;
}

float& INA219::get_bus_voltage_V_set_cvrd() {
    
    bus_voltage_V = 0;
    
    bus_voltage_reg = connection->read_register(REG_BUSVOLTAGE);
    
    if ((bus_voltage_reg & 0b0000000000000010) == 0b0000000000000000)
        conversion_ready = false;
    else
    {
        conversion_ready = true;

        bus_voltage_reg_shifted = (bus_voltage_reg >> 3);
        
        bus_voltage_V = bus_voltage_reg_shifted * 0.004 ;// 0.0039072; //one LSB of bus voltage is 4 mV
    }

    return bus_voltage_V;
}


void INA219::read_power_reg()
{
    connection->read_register(REG_POWER);
    return;
}


