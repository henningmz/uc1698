 /*
    uc1698.cpp - Library for uc1698 on Arduino Due
    created by henningmz
    and released into the public domain


    Now this is 
    it's an RGB controller. But instead of writing one pixel with it's rgv-values with every two 8 bit writes
    it's writing 3 pixels (5 bit, 6 bit, 5 bit) in monochrome mode

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

	Note: All of these pins correspond to the due's PIO Port C.
          that can bei set (REG_PIOC_SODR) and cleared (REG_PIOC_CODR)
 */


#include <Arduino.h>

// Sets the Pins connected to the display bus (33 - 40) up to output mode
// in order to write data to the display
void uc1698::pinsToOutput() {
    REG_PIOC_OER = 0b11111111 << 1;
}

// Sets the Pins connected to the display bus (33 - 40) up to input mode
// in order to read data from the controller
void uc1698::pinsToInput() {
    REG_PIOC_ODR = 0b11111111 << 1;
    
    // Enable PIO C clock, see https://forums.adafruit.com/viewtopic.php?f=25&t=109859
    if ((PMC->PMC_PCSR0 & (0x01u << ID_PIOC)) != (0x01u << ID_PIOC)) {
        PMC->PMC_PCER0 = PMC->PMC_PCSR0 | 0x01u << ID_PIOC;
    }
}

// Shorthand for an idle cycle for the Arduino
void uc1698::nop(uint times) {
    for(int i = 0; i < times; i++) {      
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



// Basic Data and Commands Writes
// nmmbered [x] and sorted according to the datasheet at
// https://www.buydisplay.com/download/ic/UC1698.pdf

// [1*] Write Data To Display Memory
void uc1698::writeSeq(uint8_t data) {
    this->pinsToOutput();

    REG_PIOC_CODR = 0b11111111 << 1;
    REG_PIOC_SODR = data << 1;
    
    this->setWR(0);
    this->nop(5);       // Delay 5 cycles
    this->setWR(1);
}

// [1] Send Data To Display
/*void uc1698::writeData(uint8_t data) {
    this->setCS(0);     // Chip select (0 = Select)
    this->setCD(1);     // Set Data (0 = Command, 1 = Data)
    this->writeSeq(data);
    this->setCS(1);
}*/

void uc1698::writeData(uint16_t data) {
    this->setCS(0);     // Chip select (0 = Select)
    this->setCD(1);     // Set Data (0 = Command, 1 = Data)

    uint8_t part1 = (uint8_t)(data >> 8);
    uint8_t part2 = (uint8_t)data;

    this->writeSeq(part1);
    this->writeSeq(part2);

    this->setCS(1);
}

// [-] Send Command To Display
void uc1698::writeCommand(uint8_t data) {
    this->setCS(0);     // Chip select (0 = Select)
    this->setCD(0);     // Set Command (0 = Command, 1 = Data)
    this->writeSeq(data);
    this->setCS(1);
    //delay(1);
}

// [2*] Reads data from the 32-bit wide Arduino port) 
// and returns 8 bits of data (uc1698 controller's bus width)
uint8_t uc1698::read() {
    this->pinsToInput();

    this->setRD(0);
    this->nop(3);       // Delay 5 cycles
	this->setRD(1);

    uint32_t dataFromPort = REG_PIOC_PDSR;  // Reads the data from the 32 bit PIO Port C
    uint8_t data = dataFromPort >> 1;       // Takes only the C1-C8 

    return data;
}

// [2] Read Data From Display Memory
uint16_t uc1698::readData() {
    this->setCS(0);
    this->setCD(1);     // Set Data (0 = Command, 1 = Data)

    this->read();                   // First read, returns dummy value, we don't use that
    uint8_t part1 = this->read();   // Second Read, returns 1,5 pixels (8 bit data of 16 bit data)
    uint8_t part2 = this->read();   // Third Read, returns second 1,5 pixels (8bit)
    
    uint16_t data = ((uint16_t)part1 << 8) | part2;

    this->setCS(1);
    return data;
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
// not implemented


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
    this->writeSeq(0b01010000 + msb);
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
void uc1698::setLCDMappingControl(bool mirrorX, bool mirrorY) {
    _isYMirrored = mirrorY;
    this->writeCommand(0b11000000 + 0b100 * mirrorX + 0b10 * mirrorY);
}

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
// not implemented

// [28] Set Partial Display Start
// not implemented

// [29] Set Partial Display End
// not implemented

// [30-40] Window Program and MTP
// not implemented







/**
 * [uc1698::init description]
 */
void uc1698::init() {
    this->initConnection();
    this->initDisplay();
}

/**
 * [uc1698::initConnection description]
 */
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
    delay(500);

    this->setRST(1);
    delay(500);

    this->systemReset();


    delay(1000);

    //this->setPowerControl();

    this->writeCommand(0x28 | 3);           //0x28 = power control command (13nF<LCD<=22nF; Internal Vlcd(*10))
    this->writeCommand(0x24 | 1);           //0x24 = temperature control (-0.05%/C)


    this->setLCDMappingControl(0, 1);
/*
    this->writeCommand(0xc0 | 2);           //0x20 = LCD Mappiing Control (SEG1-->SEG384; COM160-->COM1)

    this->writeCommand(0x88 | 3);           //0x88 = RAM Address Control (up to down, automatically +1)
*/
   /* this->writeCommand(0xD8 | 5);           //COM Scan Function (LRC: AEBCD--AEBCD;  Disable FRC;   Enable SEG PWM)

    this->writeCommand(0xA0 | 1);           //Line Rate (Frame frequency 30.5 KIPS)

    this->writeCommand(0xE8 | 3);           //LCD Bias Ratio (Bias 1/12)

    this->writeCommand(0xF1);               //Set COM End
    this->writeCommand(159);

    this->writeCommand(0xC8);               //Set N-Line Inversion

    this->writeCommand(0x00);               //(Disable NIV)

    this->writeCommand(0x84);               //Set Partial Display Control (Disable Partical function)
*/
    this->setVBiasPotentiometer(127);

    this->setInverseDisplayEnable(false);

    this->setColorPattern();
    this->setColorMode();                       // Set Color Mode to 64k
    
    this->setDisplayEnable(true);               // Sets Green Enhance Mode to Disabled, Gray Shade to On/Off and Sleep Mode to !true

    delay(500);
    this->fillScreen(0);

}

/**
 * [uc1698::xToColumn description]
 * @param  x [description]
 * @return   [description]
 */
uint8_t uc1698::xToColumn(int x)
{
    if (_isYMirrored == 0)
    {
        // for setLCDMappingControl(x,0)
        return (int) (37 + x / 3);
    }
    else
    {
        // for setLCDMappingControl(x,1)
        return (int) (37 + (x + 2) / 3);
    }
}


/**
 * [uc1698::xToColumnPosition description]
 * @param  x [description]
 * @return   [description]
 */
uint8_t uc1698::xToColumnPosition(int x)
{
    if (_isYMirrored == 0)
    {
        // for setLCDMappingControl(x,0)
        return (x % 3);    
    }
    else
    {
        // for setLCDMappingControl(x,1)
        int remainder = x % 3;
        switch(remainder) {
            case 0:
                return 0;
            case 1:
                return 2;
            case 2:
                return 1;
        }
    }
}




// with every write of 16 bit data, in 2 blocks of 8 bit,
// 3 adjacent pixels are written

/**
 * [uc1698::drawPixelTriplet description]
 * @param pixel1State [description]
 * @param pixel2State [description]
 * @param pixel3State [description]
 */
void uc1698::drawPixelTriplet(bool pixel1State, bool pixel2State, bool pixel3State)
{
    this->writeData(
          0b0000000000000000 
        + 0b1111100000000000 * pixel1State 
        + 0b0000011111100000 * pixel2State 
        + 0b0000000000011111 * pixel3State
    );
}



// Override of AdafruitGFX' drawPixel function
// 
// Converts the given x-y-coordinates to the row/col+pos-ccordinates the uc1698 operates in
// Reads the state/color the three pixels are currently in.
// Compares that 
// 
// 
// 
 
/**
 * [uc1698::drawPixel description]
 * @param x     [description]
 * @param y     [description]
 * @param color [description]
 */
void uc1698::drawPixel(int16_t x, int16_t y, uint16_t color16)
{
    // Converts the 16 bit color to simple monochrome 
    // (0x00 = false = black, everything else = true = white)
    bool color = (bool) color16;

    // Convert X-Y values to rows, columns and position in column (0,1,2)
    int row = y;
    int col = this->xToColumn(x);
    int colPos = this->xToColumnPosition(x);

    // Point controller to that address
    this->setColumnAddress(col);
    this->setRowAddress(row);

    // Read current pixel state from there
    uint16_t current3PixelState = this->readData();


    uint16_t newPixel = 0;
    uint16_t new3PixelState = current3PixelState;

    // Determine which pixel should be drawn
    switch(colPos) {
        case 0:
            newPixel = 0b1111100000000000;
            break;
        case 1:
            newPixel = 0b0000011111100000;
            break;
        case 2:
            newPixel = 0b0000000000011111;
            break;
    }


    // Add new pixel to old (3) pixel state
    if (color) {
        new3PixelState = current3PixelState | newPixel;
    } else {
        new3PixelState = current3PixelState & ~newPixel;
    }

    // Point controller (again) to current address
    this->setColumnAddress(col);
    this->setRowAddress(row);

    // Write new (3) pixel state
    this->writeData(new3PixelState);
}

/**
 * [uc1698::fillScreen description]
 * @param color the color the screen is to be colored in (false = white, true = black)
 */
void uc1698::fillScreen(bool color) {
  uint i,j;
    for (int i = 0; i < 160; i++)
    {    
        for(j = 37; j < 91; j++)
        { 
            this->setRowAddress(i);
            this->setColumnAddress(j);
            this->drawPixelTriplet(color, color, color); 
        }
    }
}