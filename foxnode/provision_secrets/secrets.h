#pragma once
#include <Arduino.h>

enum class WifiProfile : uint8_t { NTP = 0, UAS6 = 1 };

bool secretsInit();

bool secretsHasWiFi(WifiProfile profile);
bool secretsGetWiFi(WifiProfile profile, String &ssid, String &psk);
bool secretsSetWiFi(WifiProfile profile, const String &ssid, const String &psk);

bool secretsHasMtls();
bool secretsGetMtls(String &caPem, String &clientCertPem, String &clientKeyPem);
bool secretsSetMtls(const String &caPem, const String &clientCertPem, const String &clientKeyPem);

void secretsClearAll();
