#ifndef HDC1080Wrapper_h
#define HDC1080Wrapper_h

#include <Wire.h>
#include "GuL_TI_Humidity_HDC10XX.h"

namespace GuL {

    class HDC1080Wrapper {
    private:
        GuL_TI_Humidity_HDC10XX hdc;

    public:
        HDC1080Wrapper(TwoWire &wire);
        
        // Initialization and configuration
        void begin();
        void configure();

        // Read functions
        float readTemperature();
        float readHumidity();

        // Acquisition control
        void startAcquisition();
        uint16_t getConversionTime();
    };

}

#endif
