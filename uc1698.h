 /*
    uc1698.h - Library for uc1698 on Arduino Due
    created by Henning Munzel
    and released into the public domain
 */




#ifndef uc1698_h
#define uc1698_h

#include "Arduino.h"

#include "Adafruit_GFX.h"

//class uc1698 {
class uc1698 : public Adafruit_GFX {



    public:
        uc1698(uint8_t width, uint8_t height) : Adafruit_GFX(width, height) { }

        void init();
        void initConnection();
        void initDisplay();

        void nop(uint times);

        void reset();

        void setVBiasPotentiometer(uint8_t vBiasPotentiometer);
        void setAllPixeslsOn(bool allPixelsOn);
        void setInverseDisplayEnable(bool inverseDisplayEnable);
        void setDisplayEnable (bool sleepMode);


        // Adafruit GFX overrides

        void drawPixel(int16_t x, int16_t y, uint16_t color);
        void fillScreen(bool color);
        
        // public for Test
        void setColumnAddress(int column);                                      // [4]
        void setRowAddress(int row);
        void writeData(uint16_t data);

    private:
        bool _isYMirrored = 0;


        //int _displayRows = 160;


        int _temperatureCompensation = 0;   // 0: 0.00%/°C, 5: 0.05%/°C, 10: 0.10%/°C, 15: 0.15%/°C, 
        bool _highCapacitance = 0;          // 0: LCD <= 13nF, 1: 13nF <= LCD <= 2nF
        bool _internalPump = 1;             // 0: External V_LCD, 1: Internal V_LCD


        void setRST(bool RSTstate);
        void setCS(bool chipSelect);
        void setCD(bool commandData);
        void setWR(bool writeRead);
        void setRD(bool read);

        void pinsToOutput();
        void pinsToInput();


        void writeSeq(uint8_t data);                                               // [1]
        void writeCommand(uint8_t data);
        //void writeData(uint8_t data);


        uint8_t read();                                                         // [2*]
        uint16_t readData();                                                     // [2]
        //uint8_t readStatus();                                                   // [3]




        void setTemperatureCompensation(int temperatureCompensation);           // [5]
        void setPowerControl();                                                 // [6]

        void setRAMAddressControl();                                            // [12]

        void setLCDMappingControl(bool mirrorX, bool mirrorY);                  // [18]

        void setScrollLine(int line);

        void setColorPattern();                                                 // [20]
        void setColorMode();                                                    // [21]


        void systemReset();                                                     // [23]
        void NOP();                                                             // [24]


        uint8_t xToColumn(int x);
        uint8_t xToColumnPosition(int x);



        void drawPixelTriplet(bool pixel1State, bool pixel2State, bool pixel3State);
  

        void displayAddress();
};

#endif
