#include <GuL_TI_Humidity_HDC10XX.h>

#pragma once

namespace GuL
{

    class HDC1010 : public GuL_TI_Humidity_HDC10XX
    {
    private:
        uint8_t _ddrynPin;

    public:
        HDC1010(TwoWire &wire, uint8_t addr = (1 << 6));
        void setDRDYnPin(uint8_t pin);
        bool isAcquisitionReady();
    };
} // namespace GuL

namespace GuL
{
    HDC1010::HDC1010(TwoWire &wire, uint8_t addr) : GuL_TI_Humidity_HDC10XX(wire)
    {
        _addr = addr;
        if (addr < (1 << 6) || addr > (1 << 6 | 0b11))
        {
            _addr = (1 << 6);
        }
        _name = "HDC1010";
    }

    void HDC1010::setDRDYnPin(uint8_t pin)
    {
        _ddrynPin = pin;
        pinMode(_ddrynPin, INPUT_PULLUP);
    }

    bool HDC1010::isAcquisitionReady()
    {
        if (_ddrynPin == 0)
        {
            return false;
        }
        return digitalRead(_ddrynPin) == LOW;
    }

} // namespace GuL
