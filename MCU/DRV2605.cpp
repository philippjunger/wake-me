/***************************************
Based on Adafruit's library for the DRV2605

changed and extended by Lukas Hokenmaier
****************************************/


#include "DRV2605.h"


DRV2605::DRV2605() {
  // Constructor
}

boolean DRV2605::begin(TwoWire &wirePort, uint32_t i2cSpeed, uint8_t i2caddr) {
  
  _i2cPort = &wirePort; //Grab which port the user wants us to use

  _i2cPort->begin();
  _i2cPort->setClock(i2cSpeed);

  _i2caddr = i2caddr;
  
  setup();
  
  // Step 1: Initial Communication and Verification
  // Check that a DRV2605 is connected
  if (!runDiagnose()) {
    // Error -- Status does not match expected status.
    // This may mean there is a physical connectivity problem (broken wire, unpowered, etc).
    return false;
  }
    
  return true;
  
}

void DRV2605::setup() {
  
  writeRegister8(DRV2605_REG_MODE, DRV2605_MODE_INTTRIG); // out of standby
  writeRegister8(DRV2605_REG_RTPIN, 0x00); // no real-time-playback
  writeRegister8(DRV2605_REG_OVERDRIVE, 0); // no overdrive
  writeRegister8(DRV2605_REG_SUSTAINPOS, 0);
  writeRegister8(DRV2605_REG_SUSTAINNEG, 0);
  writeRegister8(DRV2605_REG_BREAK, 0);
  writeRegister8(DRV2605_REG_AUDIOMAX, 0x64);
  writeRegister8(DRV2605_REG_LIBRARY, 2); // Select Library for ERM Motor (3V rated voltage, 3V over drive)
  
  // Set ERM open loop
  // turn off N_ERM_LRA
  writeRegister8(DRV2605_REG_FEEDBACK, readRegister8(DRV2605_REG_FEEDBACK) & 0x7F);
  // turn on ERM_OPEN_LOOP
  writeRegister8(DRV2605_REG_CONTROL3, readRegister8(DRV2605_REG_CONTROL3) | 0x20);
}

void DRV2605::standby() {
  writeRegister8(DRV2605_REG_MODE, B01000000);
}

void DRV2605::go() {
  writeRegister8(DRV2605_REG_GO, 1);
}

void DRV2605::stop() {
  writeRegister8(DRV2605_REG_GO, 0);
}

void DRV2605::power() {
    writeRegister8(DRV2605_REG_MODE, DRV2605_MODE_INTTRIG);
}   

boolean DRV2605::runDiagnose() {
  // Wake DRV2605
  power();
  // Set diagnose mode
  writeRegister8(DRV2605_REG_MODE, DRV2605_MODE_DIAGNOSE);

  // Set Go for Diagnose
  go();

  // Wait until GO Bit becomes 0
  while (readRegister8(DRV2605_REG_GO) == 1){}

  // Check Statusregister

  byte x = readRegister8(DRV2605_REG_STATUS);

  if (x != B11100000) {

    // Repeat Diagnose to go for sure
    // Set Go for Diagnose
    go();

    // Wait until GO Bit becomes 0
    while (readRegister8(DRV2605_REG_GO) == 1);
    
    byte x = readRegister8(DRV2605_REG_STATUS);
    
    if (x != B11100000) {
      
      standby();
      return false; 
    }
  }
  
  // Take DRV2605 to standby
  standby();
  
  return true;
  
}

// Diese Funktion kann bis zu 8 mal hintereinander aufegerufen werden.
// Beim nächsten GO Bit werden alle gesetzten Einträge gespielt. 
void DRV2605::setWaveform(uint8_t slot, uint8_t w) {
  writeRegister8(DRV2605_REG_WAVESEQ1 + slot, w);
}

uint8_t DRV2605::readRegister8(uint8_t reg) {
  _i2cPort->beginTransmission(_i2caddr);
  _i2cPort->write(reg);
  byte error = _i2cPort->endTransmission(false);
  if (error != 0) {
    return (0);
  }
  
  _i2cPort->requestFrom(_i2caddr, (uint8_t)1); // Request 1 byte
  if (_i2cPort->available())
  {
    return(_i2cPort->read());
  }

  return (0); //Fail
}

uint8_t DRV2605::writeRegister8(uint8_t reg, uint8_t value) {
  _i2cPort->beginTransmission(_i2caddr);
  
  _i2cPort->write(reg);
  _i2cPort->write(value);
  byte error = _i2cPort->endTransmission();
  if (error != 0) {
    return (0);
  }
}
