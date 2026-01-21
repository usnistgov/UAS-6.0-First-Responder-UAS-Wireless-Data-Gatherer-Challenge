#include "HDC1080Wrapper.h"
#include "HDC1080.h"  // Include the correct header file for HDC1080 class

namespace GuL {

    HDC1080Wrapper::HDC1080Wrapper(TwoWire &wire) : hdc(wire) {}

    void HDC1080Wrapper::begin() {
        // Initialization or checking for the sensor (if needed)
    }

    void HDC1080Wrapper::configure() {
        // Set sensor configuration using methods from HDC1080
        hdc.resetConfiguration();
        hdc.enableHeater();
        hdc.setHumidityResolution(GuL::HDC1080::HumidityMeasurementResolution::HUM_RES_14BIT);
        hdc.setTemperaturResolution(GuL::HDC1080::TemperatureMeasurementResolution::TEMP_RES_14BIT);
        hdc.setAcquisitionMode(GuL::HDC1080::AcquisitionModes::BOTH_CHANNEL);
    }

    void HDC1080Wrapper::startAcquisition() {
        hdc.startAcquisition(GuL::HDC1080::Channel::BOTH);
        delay(hdc.getConversionTime(GuL::HDC1080::Channel::BOTH) / 1000);
    }

    float HDC1080Wrapper::readTemperature() {
        return hdc.getTemperature();
    }

    float HDC1080Wrapper::readHumidity() {
        return hdc.getHumidity();
    }

    uint16_t HDC1080Wrapper::getConversionTime() {
        return hdc.getConversionTime(GuL::HDC1080::Channel::BOTH);
    }
}
