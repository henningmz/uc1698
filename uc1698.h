 /*
    uc1698.h - Library for uc1698 on Arduino Due
    created by Henning Munzel
    and released into the public domain
 */

#ifndef uc1698_h
#define uc1698_h

#include "Arduino.h"


class uc1698 {
    public:
        uc1698();

        void initConnection();
        void initDisplay();

        void nop(uint times);

        void test();
        void reset();

        void setVBiasPotentiometer(uint8_t vBiasPotentiometer);
        void setAllPixeslsOn(bool allPixelsOn);
        void setInverseDisplayEnable(bool inverseDisplayEnable);
        void setDisplayEnable (bool sleepMode);


    private:
        // 33: DB0, ... 40: DB7

        // 47: Reset pin (0 = Reset)
        // 48: Write Clock [WR] (0 = Write Data, 1 = Wait for next write)
        // 49: Read Clock [RD] (0 = Read Data, 1 = Wait for next read)
        // 50: Control Data pin (0 = Control Data, 1 = Display Data)
        // 51: Chip Select

        int _displayRows = 160;


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

        uint8_t read();                                                         // READ

        void write(uint8_t data);                                               // [1]
        void writeCommand(uint8_t data);
        void writeData(uint8_t data);

        void setColumnAddress(int column);                                      // [4]
        void setTemperatureCompensation(int temperatureCompensation);           // [5]
        void setPowerControl();                                                 // [6]

        void setRAMAddressControl();                                            // [12]

        void setRowAddress(int row);
        void setScrollLine(int line);

        void setColorPattern();                                                 // [20]
        void setColorMode();                                                    // [21]


        void systemReset();                                                     // [23]
        void NOP();                                                             // [24]

        void setCOMEnd();                                                       // [27]



        void drawPixelTriplet(bool pixel1State, bool pixel2State, bool pixel3State);

        void displayTestPattern();

        void displayWhite();
        void displayBlack();

        void displayAddress();
};

#endif
