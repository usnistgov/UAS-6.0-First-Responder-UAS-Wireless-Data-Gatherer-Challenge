#include "secrets.h"
#include <Preferences.h>

static Preferences prefs;
static const char *NS = "fox_secrets";

static const size_t MAX_SSID = 64;
static const size_t MAX_PSK  = 128;
static const size_t MAX_PEM  = 4096;

bool secretsInit() { return prefs.begin(NS, false); }

static bool getKey(const char *key, String &out, size_t maxLen) {
  if (!prefs.isKey(key)) return false;
  out = prefs.getString(key, "");
  if (out.length() == 0) return false;
  if (out.length() > maxLen) return false;
  return true;
}

static bool setKey(const char *key, const String &val, size_t maxLen) {
  if (val.length() == 0) return false;
  if (val.length() > maxLen) return false;
  return prefs.putString(key, val) > 0;
}

static void wifiKeys(WifiProfile p, const char* &ssidKey, const char* &pskKey) {
  if (p == WifiProfile::NTP) {
    ssidKey = "wifi_ntp_ssid";
    pskKey  = "wifi_ntp_psk";
  } else {
    ssidKey = "wifi_uas6_ssid";
    pskKey  = "wifi_uas6_psk";
  }
}

bool secretsHasWiFi(WifiProfile profile) {
  const char *ssidKey, *pskKey;
  wifiKeys(profile, ssidKey, pskKey);
  String a, b;
  return getKey(ssidKey, a, MAX_SSID) && getKey(pskKey, b, MAX_PSK);
}

bool secretsGetWiFi(WifiProfile profile, String &ssid, String &psk) {
  const char *ssidKey, *pskKey;
  wifiKeys(profile, ssidKey, pskKey);
  if (!getKey(ssidKey, ssid, MAX_SSID)) return false;
  if (!getKey(pskKey,  psk,  MAX_PSK))  return false;
  return true;
}

bool secretsSetWiFi(WifiProfile profile, const String &ssid, const String &psk) {
  const char *ssidKey, *pskKey;
  wifiKeys(profile, ssidKey, pskKey);
  if (!setKey(ssidKey, ssid, MAX_SSID)) return false;
  if (!setKey(pskKey,  psk,  MAX_PSK))  return false;
  return true;
}

// mTLS functions can remain exactly as before
bool secretsHasMtls() {
  String a, b, c;
  return getKey("mtls_ca", a, MAX_PEM) &&
         getKey("mtls_cert", b, MAX_PEM) &&
         getKey("mtls_key", c, MAX_PEM);
}

bool secretsGetMtls(String &caPem, String &clientCertPem, String &clientKeyPem) {
  if (!getKey("mtls_ca",   caPem,         MAX_PEM)) return false;
  if (!getKey("mtls_cert", clientCertPem, MAX_PEM)) return false;
  if (!getKey("mtls_key",  clientKeyPem,  MAX_PEM)) return false;
  return true;
}

bool secretsSetMtls(const String &caPem, const String &clientCertPem, const String &clientKeyPem) {
  if (!setKey("mtls_ca",   caPem,         MAX_PEM)) return false;
  if (!setKey("mtls_cert", clientCertPem, MAX_PEM)) return false;
  if (!setKey("mtls_key",  clientKeyPem,  MAX_PEM)) return false;
  return true;
}

void secretsClearAll() { prefs.clear(); }
