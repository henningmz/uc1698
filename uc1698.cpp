 /*
    uc1698.cpp - Library for uc1698 on Arduino Due
    created by Henning Munzel
    and released into the public domain

    tested with Arduino Due and an ERC160160FS-2 and ERC160160SBS-2 from buydisplay.com

    https://www.buydisplay.com/default/3-inch-graphic-160x160-lcd-display-controller-uc1698-module-black-on-white
    https://www.buydisplay.com/default/3-inch-led-backlight-display-160x160-lcm-module-graphic-white-on-blue

    The driver assumes display/controller are connected to the Arduino Due via these pins and in accordance with
    the "Interfacing Document" found at https://www.buydisplay.com/download/interfacing/ERC160160-2_Interfacing.pdf

        33: DB0
        34: DB1 
            ...
        40: DB7

        47: Reset pin (0 = Reset)
        48: Write Clock [WR] (0 = Write Data, 1 = Wait for next write)
        49: Read Clock [RD] (0 = Read Data, 1 = Wait for next read)
        50: Control Data pin (0 = Control Data, 1 = Display Data)
        51: Chip Select

 */

#include <Arduino.h>
#include <uc1698.h>



uc1698::uc1698() { }


// Sets the Pins connected to the display bus (33 - 40) up to output mode
// in order to write data to the display

void uc1698::pinsToOutput() {
    REG_PIOC_OER = 0b11111111 << 1;
}


// Sets the Pins connected to the display bus (33 - 40) up to input mode
// in order to read data from the display

void uc1698::pinsToInput() {
    REG_PIOC_ODR = 0b11111111 << 1;
}


// Shorthand for an idle cycle for the Arduino
//

void uc1698::nop(uint times) {
    uint i;
    for(i=0; i<times; i++) {      
        __asm__("nop\n\t"); 
    }
}



// Toggles whether the the chip is selected (i.e. takes data)
// the Pin is active low, so 0 = Chip selected

void uc1698::setCS(bool chipSelect) {   
    //digitalWrite(_CSPin, chipSelect); 
    if (chipSelect) {
        REG_PIOC_SODR = 1 << 12;
    } else {
        REG_PIOC_CODR = 1 << 12;
    }
}


// Toggles whether the next instruction to the controller is a command 
// (i.e. setting a row address) or display data
// Pin is active low, so 0 = Command, 1 = Data

void uc1698::setCD(bool command) { 
    //digitalWrite(_CDPin, commandData); 
    if (command) {
        REG_PIOC_SODR = 1 << 13;
    } else {
        REG_PIOC_CODR = 1 << 13;
    }
}


// Toggles the read clock pin (RD) 
// Pin is active low, so 0 = Read, 1 = Wait for Read

void uc1698::setRD(bool read) {
    //digitalWrite(_RDPin, read);
    if (read) {
        REG_PIOC_SODR = 1 << 14;
    } else {
        REG_PIOC_CODR = 1 << 14;
    }
}

// Toggles the write clock pin (WR) 
// Pin is active low, so 0 = Write, 1 = Wait for Write

void uc1698::setWR(bool write) {
    //digitalWrite(_WRPin, writeRead);
    if (write) {
        REG_PIOC_SODR = 1 << 15;
    } else {
        REG_PIOC_CODR = 1 << 15;
    }
}


// Toggles the reset pin
// Pin is active low, so 0 = Reset, 1 = Run as ususal

void uc1698::setRST(bool RSTstate) {
    //digitalWrite(_RSTPin, RSTstate);
    if (RSTstate) {
        REG_PIOC_SODR = 1 << 16;
    } else {
        REG_PIOC_CODR = 1 << 16;
    }

}


// Reads data from the 32-bit wide Arduino port) 
// and returns 8 bits of data (uc1698 controller's bus width)

uint8_t uc1698::read() {
    this->pinsToInput();
    
    this->setRD(0);
    this->nop(5);       // Delay 5 cycles
    this->setRD(1);

    uint32_t dataFromPort = REG_PIOC_PDSR;
    uint8_t data = dataFromPort >> 1;
    
    return data;
}


// Basic Data and Commands Writes
// nmmbered [x] and sorted according to the datasheet at
// https://www.buydisplay.com/download/ic/UC1698.pdf

// [1*] Write Data To Display Memory
void uc1698::write(uint8_t data) {
    this->pinsToOutput();

    REG_PIOC_CODR = 0b11111111 << 1;
    REG_PIOC_SODR = data << 1;
    
    this->setWR(0);
    this->nop(5);       // Delay 5 cycles
    this->setWR(1);
}

// [1] Send Data To Display
void uc1698::writeData(uint8_t data) {
    this->setCS(0);     // Chip select (0 = Select)
    this->setCD(1);     // Set Data (0 = Command, 1 = Data)
    this->write(data);
    this->setCS(1);
}

// [-] Send Command To Display
void uc1698::writeCommand(uint8_t data) {
    this->setCS(0);     // Chip select (0 = Select)
    this->setCD(0);     // Set Command (0 = Command, 1 = Data)
    this->write(data);
    this->setCS(1);
    delay(1);
}


// [4] Set Column Address
void uc1698::setColumnAddress(int column) {
    // takes the address of the colum and divides it into two sets
    // of data (most significant bits and least signifant bits)
    // to be send as individual writes to the controller

    uint8_t msb, lsb = 0;

    msb = bitRead(column, 6) * 0b100
        + bitRead(column, 5) * 0b10
        + bitRead(column, 4);

    lsb = bitRead(column, 3) * 0b1000 
        + bitRead(column, 2) * 0b100 
        + bitRead(column, 1) * 0b10
        + bitRead(column, 0);

    this->writeCommand(0b00000000 + lsb);
    this->writeCommand(0b00010000 + msb);
}

// [5] Set Temperature Compensation
void uc1698::setTemperatureCompensation(int temperatureCompensation) {
    if (temperatureCompensation == 0 || temperatureCompensation == 5 || temperatureCompensation == 10 || temperatureCompensation == 15) {
        _temperatureCompensation = temperatureCompensation;
    }

    switch (temperatureCompensation) {
        case 0:
            this->writeCommand(0b00100100);
            break;
        case 5:
            this->writeCommand(0b00100101);
            break;
        case 10:
            this->writeCommand(0b00100110);
            break;
        case 15:
            this->writeCommand(0b00100111);
            break;
    }
    
}


// [6] Set Power Control
// Unsafe, internal pump should only be changed when the controller is in a reset state
void uc1698::setPowerControl() {
    this->writeCommand(0b00101011);
}


// [7] Set Advanced Program Control
// not implemented yet


// [8] Set Scroll Line
void uc1698::setScrollLine(int line) {
    uint8_t lsb, msb = 0;

    msb = bitRead(line, 7) * 0b1000
        + bitRead(line, 6) * 0b100
        + bitRead(line, 5) * 0b10
        + bitRead(line, 4);

    lsb = bitRead(line, 3) * 0b1000 
        + bitRead(line, 2) * 0b100 
        + bitRead(line, 1) * 0b10
        + bitRead(line, 0);

    this->writeCommand(0b01000000 + lsb);
    this->write(0b01010000 + msb);
}


// [9] Set Row Address
void uc1698::setRowAddress(int row) {
    uint8_t lsb, msb = 0;

    msb = bitRead(row, 7) * 0b1000
        + bitRead(row, 6) * 0b100
        + bitRead(row, 5) * 0b10
        + bitRead(row, 4);

    lsb = bitRead(row, 3) * 0b1000 
        + bitRead(row, 2) * 0b100 
        + bitRead(row, 1) * 0b10
        + bitRead(row, 0);

    this->writeCommand(0b01100000 + lsb);
    this->writeCommand(0b01110000 + msb);
}


// [10] Set VBias Potentiometer
void uc1698::setVBiasPotentiometer(uint8_t vBiasPotentiometer) {
    this->writeCommand(0b10000001);
    this->writeCommand(vBiasPotentiometer);
}


// [11] Set Partial Display Control
// not implemented


// [12] Set RAM Address Control
void uc1698::setRAMAddressControl() {
    this->writeCommand(0b10001000);
}


// [13] Set Fixed Lines
// not implemented


// [14] Set Line Rate
// not implemented


// [15] Set All Pixels On
void uc1698::setAllPixeslsOn(bool allPixeslsOn) {
    this->writeCommand(0b10100100 + allPixeslsOn);
}

// [16] Set Inverse Display
void uc1698::setInverseDisplayEnable(bool inverseDisplayEnable) {
    this->writeCommand(0b10100110 + inverseDisplayEnable);
}

// [17] Set Display Enable
void uc1698::setDisplayEnable(bool sleepMode) {
    this->writeCommand(0b10101100 + sleepMode);
}


// [18] Set LCD Mapping Control
//void uc1698::setLCDMappingControl() {}


// [19] Set N-Line Inversion
// not implemented


// [20] Set Color Pattern
void uc1698::setColorPattern() {
    this->writeCommand(0b11010001);     // sets Color Pattern to R-G-B
}

// [21] Set Color Mode
void uc1698::setColorMode() {
        this->writeCommand(0b11010100);     // sets Color Mode to 64k
}

// [22] Set COM Scan Function
// not implemented


// [23] System Reset
void uc1698::systemReset() {
    this->writeCommand(0b11100010);
    delay(1);
}


// [24] NOP
void uc1698::NOP() {
    this->writeCommand(0b11100010);
}


// [25] Set Test Control
// not implemented, and should not be used
// (for production testing only)


// [26] Set LCD Bias Ratio
// not implemented


// [27] Set COM End
void uc1698::setCOMEnd() {
    this->writeCommand(0b11110001);
    this->writeCommand(0b00000000 + _displayRows);
}

// not implemented


// [28] Set Partial Display Start
// not implemented


// [29] Set Partial Display End
// not implemented


// [30-40] Window Program and MTP
// not implemented





/*
//  Some functions for convenience
*/


// runs the PowerUp procedure
/*void uc1698::powerUp() {
    this->reset();
    //this->setTempCompensation();
    //this->setLCDMappingControl
    //this->setLineRate
    this->setColorMode();
    //this->setLCDBiasRatio
    this->setColorMode();
    
    // write display ram
    //this->writeData();

    this->setDisplayEnable(1, 1, 0);
}*/


void uc1698::initConnection() {
    REG_PIOC_OER = 0b11111 << 12;

    this->setCS(1);
    this->setRST(1);
    this->setRD(1);
    this->setWR(1);
    this->setCD(1);

    this->pinsToOutput();
}


void uc1698::initDisplay() {

    this->setRST(0);
    this->setCS(0);
    delay(150);



    this->setRST(1);
    delay(100);

    this->systemReset();


    delay(500);

    //this->setPowerControl();


    this->writeCommand(0x28 | 3);           //0x28 = power control command (13nF<LCD<=22nF; Internal Vlcd(*10))

    this->writeCommand(0x24 | 1);           //0x24 = temperature control (-0.05%/C)

    this->writeCommand(0xc0 | 2);           //0x20 = LCD Mappiing Control (SEG1-->SEG384; COM160-->COM1)

    this->writeCommand(0x88 | 3);           //0x88 = RAM Address Control (up to down, automatically +1)

    this->writeCommand(0xD8 | 5);           //COM Scan Function (LRC: AEBCD--AEBCD;  Disable FRC;   Enable SEG PWM)

    this->writeCommand(0xA0 | 1);           //Line Rate (Frame frequency 30.5 KIPS)

    this->writeCommand(0xE8 | 3);           //LCD Bias Ratio (Bias 1/12)

    this->writeCommand(0xF1);               //Set COM End
    this->writeCommand(159);

    this->writeCommand(0xC8);               //Set N-Line Inversion

    this->writeCommand(0x00);               //(Disable NIV)

    this->writeCommand(0x84);               //Set Partial Display Control (Disable Partical function)

    this->setVBiasPotentiometer(127);

    this->setInverseDisplayEnable(false);

    this->setColorPattern();
    this->setColorMode();                       // Set Color Mode to 64k
    
    this->setDisplayEnable(true);               // Sets Green Enhance Mode to Disabled, Gray Shade to On/Off and Sleep Mode to !true





}



void uc1698::drawPixelTriplet(bool pixel1State, bool pixel2State, bool pixel3State)
{
    this->writeData(0b00000000 + 0b11111000 * pixel1State + 0b111 * pixel2State);
    this->writeData(0b00000000 + 0b11100000 * pixel2State + 0b11111 * pixel3State);
}






void uc1698::displayWhite() {
  uint i,j;

  for(i=0;i<160;i++)
     {    
        this->setRowAddress(i);
        for(j=0;j<54;j++)
        { 
            
            this->drawPixelTriplet(1, 1, 1); 
            //this->writeData(0xff);
            //this->writeData(0xff);

        }
     }
}


void uc1698::displayBlack() {
  uint i,j;
  for(i=0;i<160;i++)
     {    
        this->setRowAddress(i);
        for(j=0;j<54;j++)
        { 
            this->drawPixelTriplet(0, 0, 0); 
            //this->writeData(0x00);
            //this->writeData(0x00);
        }
     }
}

void uc1698::displayTestPattern() {
  uint i,j;
  for(i=0;i<100;i++)
    {     
        this->setRowAddress(i);
        this->setColumnAddress(j);
        this->drawPixelTriplet(1, 1, 1);
        delay(50);
    }
}






void uc1698::test() {
    this->displayTestPattern();
}
