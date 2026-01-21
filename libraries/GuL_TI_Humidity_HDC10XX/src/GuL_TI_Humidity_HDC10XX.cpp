#include "GuL_TI_Humidity_HDC10XX.h"

namespace GuL
{
    GuL_TI_Humidity_HDC10XX::GuL_TI_Humidity_HDC10XX(TwoWire &wire) : _wire(wire)
    {
        this->fetchConfig();
        _name = "HDC10XX";
    }

    bool GuL_TI_Humidity_HDC10XX::startAcquisition(Channel chan)
    {
        RegisterMap reg = (chan == Channel::HUMIDITY) ? RegisterMap::HUMIDITY_REG : RegisterMap::TEMPERATURE_REG;
        return this->send(reg, std::vector<uint16_t>{});
    }

    bool GuL_TI_Humidity_HDC10XX::setAcquisitionMode(AcquisitionModes mode)
    {
        _config.fields.MODE = mode;
        return this->send(RegisterMap::CONFIGURATION_REG, std::vector<uint16_t>{_config.bits});
    }

    uint16_t GuL_TI_Humidity_HDC10XX::getConversionTime(Channel chan)
    {
        switch (chan)
        {
        case Channel::HUMIDITY:
            return (_config.fields.HRES == HumidityMeasurementResolution::HUM_RES_8BIT) ? 2500 : ((_config.fields.HRES == HumidityMeasurementResolution::HUM_RES_11BIT) ? 3850 : 6500);
            break;
        case Channel::TEMPERATURE:
            return (_config.fields.TRES == TemperatureMeasurementResolution::TEMP_RES_11BIT) ? 3650 : 6350;
        case Channel::BOTH:
            return this->getConversionTime(Channel::TEMPERATURE) + this->getConversionTime(Channel::HUMIDITY);
            break;
        }
        return 0;
    }

    bool GuL_TI_Humidity_HDC10XX::resetConfiguration()
    {
        config_t conf;
        conf.bits = 0;
        conf.fields.RST = 1;

        bool success = this->send(RegisterMap::CONFIGURATION_REG, std::vector<uint16_t>{conf.bits});
        this->fetchConfig();
        return success;
    }

    bool GuL_TI_Humidity_HDC10XX::fetchConfig()
    {
        std::vector<uint16_t> data;
        data.resize(1);
        if (!this->request(RegisterMap::CONFIGURATION_REG, data))
        {
            return false;
        }
        _config.bits = data.at(0);
        return true;
    }

    GuL_TI_Humidity_HDC10XX::BatteryStatus GuL_TI_Humidity_HDC10XX::getBatteryStatus()
    {
        this->fetchConfig();
        return (BatteryStatus)_config.fields.BTST;
    }

    bool GuL_TI_Humidity_HDC10XX::send(RegisterMap startRegister, std::vector<uint16_t> data)
    {
        _wire.beginTransmission(_addr);
        uint8_t frame[data.size() + 1];
        frame[0] = startRegister;
        for (size_t i = 0; i < data.size(); i++)
        {
            frame[2 * i + 1] = data.at(i) >> 8;
            frame[2 * i + 2] = data.at(i) & 0xFF;
        }

        _wire.write(frame, data.size() + 1);
        uint8_t r = _wire.endTransmission();

        _lastReturnState = (CommunicatioReturnState)r;
        return r == CommunicatioReturnState::COM_SUCCESS;
    }

    bool GuL_TI_Humidity_HDC10XX::request(RegisterMap, std::vector<uint16_t> &data)
    {
        uint8_t received = _wire.requestFrom(_addr, data.size() * 2);
        for (size_t i = 0; i < received; i += 2)
        {
            data[i] = _wire.read() << 8 | _wire.read();
        }
        return received == data.size() * 2;
    }

    uint64_t GuL_TI_Humidity_HDC10XX::getSerialID()
    {
        std::vector<uint16_t> data;
        data.resize(3);
        if (!this->request(RegisterMap::SERIAL_REG_0, data))
        {
            return 0;
        }
        return data.at(0) << 24 | data.at(1) << 8 | (data.at(2) >> 8 & 0xFF);
    }

    uint16_t GuL_TI_Humidity_HDC10XX::getDeviceID()
    {
        std::vector<uint16_t> data;
        data.resize(1);
        if (!this->request(RegisterMap::DEVICE_ID_REG, data))
        {
            return 0;
        }
        return data.at(0);
    }
    uint16_t GuL_TI_Humidity_HDC10XX::getManufacturerID()
    {
        std::vector<uint16_t> data;
        data.resize(1);
        if (!this->request(RegisterMap::MANUFACTURER_ID_REG, data))
        {
            return 0;
        }
        return data.at(0);
    }

    bool GuL_TI_Humidity_HDC10XX::enableHeater()
    {
        _config.fields.HEAT = 1;
        return this->send(RegisterMap::CONFIGURATION_REG, std::vector<uint16_t>{_config.bits});
    }

    bool GuL_TI_Humidity_HDC10XX::disableHeater()
    {
        _config.fields.HEAT = 1;
        return this->send(RegisterMap::CONFIGURATION_REG, std::vector<uint16_t>{_config.bits});
    }

    bool GuL_TI_Humidity_HDC10XX::heaterEnabled()
    {
        return _config.fields.HEAT;
    }

    bool GuL_TI_Humidity_HDC10XX::setHumidityResolution(HumidityMeasurementResolution res)
    {
        _config.fields.HRES = res;
        return this->send(RegisterMap::CONFIGURATION_REG, std::vector<uint16_t>{_config.bits});
    }

    bool GuL_TI_Humidity_HDC10XX::setTemperaturResolution(TemperatureMeasurementResolution res)
    {
        _config.fields.TRES = res;
        return this->send(RegisterMap::CONFIGURATION_REG, std::vector<uint16_t>{_config.bits});
    }

    float GuL_TI_Humidity_HDC10XX::getTemperature()
    {
        std::vector<uint16_t> data;
        data.resize(1);
        if (!this->request(RegisterMap::TEMPERATURE_REG, data))
        {
            return INVALID_TEMPERATURE;
        }
        return (float)data.at(0) / 65535.0f * 165.0 - 40;
    }

    float GuL_TI_Humidity_HDC10XX::getHumidity()
    {
        std::vector<uint16_t> data;
        data.resize(1);
        if (!this->request(RegisterMap::HUMIDITY_REG, data))
        {
            return INVALID_HUMIDITY;
        }
        return (float)data.at(0) / 65535.0f * 100;
    }
} // namespace GuL
