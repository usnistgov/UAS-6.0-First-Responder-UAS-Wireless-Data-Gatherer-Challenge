/**
 * GuL_TI_Humidity_HDC10XX
 * Copyright (c) 2023 Guido Lehne
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Developed for Arduino-ESP32
 * Created by Guido Lehne
 *
 */

#include <Arduino.h>
#include <vector>
#include <Wire.h>

#pragma once

namespace GuL
{
    class GuL_TI_Humidity_HDC10XX
    {

    public:
        constexpr static float INVALID_TEMPERATURE = -273.15;
        constexpr static float INVALID_HUMIDITY = -1;
        constexpr static float TYPICAL_TEMPERATURE_ACCURACY = 0.2;
        constexpr static float TYPICAL_HUMIDITY_ACCURACY = 2;

        enum CommunicatioReturnState
        {
            COM_SUCCESS = 0,
            COM_LENGTH_ERROR = 1,
            COM_ADDR_NACK = 2,
            COM_DATA_NACK = 3,
            COM_OTHER_ERROR = 4,
            COM_TIMEOUT = 5
        };

        enum BatteryStatus
        {
            BATTERY_OK = 0b0,
            BATTERY_LOW = 0b1
        };
        enum AcquisitionModes
        {
            SINGLE_CHANNEL = 0b0,
            BOTH_CHANNEL = 0b1
        };

        enum HumidityMeasurementResolution
        {
            HUM_RES_14BIT = 0b00,
            HUM_RES_11BIT = 0b01,
            HUM_RES_8BIT = 0b10
        };
        enum TemperatureMeasurementResolution
        {
            TEMP_RES_14BIT = 0b0,
            TEMP_RES_11BIT = 0b1
        };
        enum Channel
        {
            HUMIDITY,
            TEMPERATURE,
            BOTH
        };

        GuL_TI_Humidity_HDC10XX(TwoWire &wire);

        String getSensorName() { return _name; }
        bool startAcquisition(Channel chan);
        bool setAcquisitionMode(AcquisitionModes mode);
        bool resetConfiguration();
        bool enableHeater();
        bool disableHeater();
        bool heaterEnabled();
        bool setHumidityResolution(HumidityMeasurementResolution res);
        bool setTemperaturResolution(TemperatureMeasurementResolution res);
        BatteryStatus getBatteryStatus();
        uint16_t getDeviceID();
        uint16_t getManufacturerID();
        uint64_t getSerialID();
        uint16_t getConversionTime(Channel chan); // Returns the conversion time in µs!

        float getTemperature(); // in °C
        float getHumidity();    // in %

    protected:
        CommunicatioReturnState _lastReturnState;
        uint8_t _addr = 0b1000000;
        String _name;

        typedef union
        {
            struct
            {
                uint8_t RST : 1;
                uint8_t RESERVED0 : 1;
                uint8_t HEAT : 1;
                uint8_t MODE : 1;
                uint8_t BTST : 1;
                uint8_t TRES : 1;
                uint8_t HRES : 2;
                uint8_t RESERVED1 : 8;
            } fields;
            uint16_t bits;
        } config_t;
        config_t _config;

        enum RegisterMap
        {
            TEMPERATURE_REG = 0x00,
            HUMIDITY_REG = 0x01,
            CONFIGURATION_REG = 0x02,
            SERIAL_REG_0 = 0xFB,
            SERIAL_REG_1 = 0xFC,
            SERIAL_REG_2 = 0xFD,
            MANUFACTURER_ID_REG = 0xFE,
            DEVICE_ID_REG = 0xFF
        };

        bool send(RegisterMap, std::vector<uint16_t> data);

        bool request(RegisterMap, std::vector<uint16_t> &data);
        bool fetchConfig();

    private:
        TwoWire &_wire;
    };

} // namespace GuL
