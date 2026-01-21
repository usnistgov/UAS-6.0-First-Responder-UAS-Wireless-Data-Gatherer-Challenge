#include <GuL_TI_Humidity_HDC10XX.h>

#pragma once

namespace GuL
{

    class HDC1080 : public GuL_TI_Humidity_HDC10XX
    {
    public:
        HDC1080(TwoWire &wire);
    };
} // namespace GuL

namespace GuL
{
    HDC1080::HDC1080(TwoWire &wire) : GuL_TI_Humidity_HDC10XX(wire)
    {
        _addr = 0b1000000;
        _name = "HDC1080";
    }
} // namespace GuL
