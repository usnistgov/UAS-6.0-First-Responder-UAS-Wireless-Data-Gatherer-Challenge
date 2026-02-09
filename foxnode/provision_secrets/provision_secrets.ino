#include "secrets.h"

// Temporary secrets: put these in only for the one-time provision build.
// After provisioning, delete this file or keep it outside your repo.

// Sanitize PEM strings before storing in NVS.
// Fixes common copy/paste issues: CRLF, stray \r, BOM, trailing spaces, hidden chars at ends of lines.
static String sanitizePem(const char *pem) {
  String s = String(pem);

  // Normalize Windows line endings
  s.replace("\r\n", "\n");
  s.replace("\r", "\n");

  // Remove UTF-8 BOM if present (EF BB BF)
  if (s.length() >= 3 &&
      (uint8_t)s[0] == 0xEF &&
      (uint8_t)s[1] == 0xBB &&
      (uint8_t)s[2] == 0xBF) {
    s.remove(0, 3);
  }

  // Trim global whitespace
  s.trim();

  // Trim whitespace at the end of each line (and remove accidental indentation)
  String out;
  out.reserve(s.length() + 4);
  int start = 0;
  while (start < s.length()) {
    int end = s.indexOf('\n', start);
    if (end < 0) end = s.length();
    String line = s.substring(start, end);
    line.trim();
    out += line;
    out += "\n";
    start = end + 1;
  }
  // Canonicalize PEM delimiter lines (remove extra spaces people accidentally paste)
  out.replace("-----BEGIN  CERTIFICATE-----", "-----BEGIN CERTIFICATE-----");
  out.replace("-----END  CERTIFICATE-----",   "-----END CERTIFICATE-----");

  out.replace("-----BEGIN  PRIVATE KEY-----", "-----BEGIN PRIVATE KEY-----");
  out.replace("-----END  PRIVATE KEY-----",   "-----END PRIVATE KEY-----");

  out.replace("-----BEGIN  RSA PRIVATE KEY-----", "-----BEGIN RSA PRIVATE KEY-----");
  out.replace("-----END  RSA PRIVATE KEY-----",   "-----END RSA PRIVATE KEY-----");


  return out;
}

// NTP WiFi (internet connected)
static const char *PROV_NTP_SSID = "UAS_NTP";
static const char *PROV_NTP_PSK  = "whatTime!";

// UAS6 WiFi (operational network)
static const char *PROV_UAS6_SSID = "uas6";
static const char *PROV_UAS6_PSK  = "hello123";

// PEM blocks: keep them exactly as PEM with newlines.
static const char *PROV_CA_PEM = R"PEM(
-----BEGIN CERTIFICATE-----
MIIFqTCCA5GgAwIBAgIUaCk73Ughy4QCA/fDyANRvcJzW+swDQYJKoZIhvcNAQEL
BQAwZDELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAkNPMRAwDgYDVQQHDAdCb3VsZGVy
MQ0wCwYDVQQKDAROSVNUMQwwCgYDVQQLDANVQVMxGTAXBgNVBAMMEFVBUzYtSW50
ZXJuYWwtQ0EwHhcNMjYwMTMwMTc1NDQ1WhcNMzYwMTI4MTc1NDQ1WjBkMQswCQYD
VQQGEwJVUzELMAkGA1UECAwCQ08xEDAOBgNVBAcMB0JvdWxkZXIxDTALBgNVBAoM
BE5JU1QxDDAKBgNVBAsMA1VBUzEZMBcGA1UEAwwQVUFTNi1JbnRlcm5hbC1DQTCC
AiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAKh/gIDlU58C1ngvMZB2f82h
dzejpPebz9/d1KWWW49bIhL+mieqQ/TD6Y+Mzxx/r/8c9EY2BC2tw3LrMMbwU1+m
M76XE9fWVfeQx5DE2oKPCdAlChEV4ZRA7KE4YypM3SEbI8PHVaqdFstLQZA+6KfX
6K9g8wEBnVCdfX2NAhlTg2R5YuqAzC90T3PfGodHZfdAbDB1GeA0C32dldtK7nyU
ESmz4drTQrWYFNkWbNzYVu330PqAKDnLNXOr0sz7tD549pewB0Md+sxjrE5d6IDL
aw9lax/0zrhSw7Heli3H+CLxwJIT5ffRryhhPrTKvZRQN+OXuPucX5HfhgeoiqJR
94uiT4u6TWHyjMcfKTaYE9kzrhCeaSZJiivIF2yIj+aHRRlNfeWZkSmRNcgubela
JaAKs/a6zga5JVNzCL3p95jSF8zr8KCBaqzWSGH0hpx8NrO1tuBkt+GuO7Sh1rn6
+7jAX2aJGHZWOhKyFMFezobjRCN6wuzagicMlVHr3CFMBk+Vg81EPmOjUZcsEJEC
KkNjW4repDAGyJcK4GXdOhPRKO6L7bI4ISc3/FM1L7Z4z48hJ0OqvhNFKaXuauV1
Vb1UVTI6e5jsC18t7QABcyCpRUJZC39a7m5J2IqUb7j4ffzVSxhmXSREhu85JIfM
cn/WgsyxR0ehP5ncvBwhAgMBAAGjUzBRMB0GA1UdDgQWBBTFJSf/FJbc/iMKB6cf
tpCJS6DYbDAfBgNVHSMEGDAWgBTFJSf/FJbc/iMKB6cftpCJS6DYbDAPBgNVHRMB
Af8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4ICAQCkQiT9uaihEVFmiaa2x1XQ7Bh1
L7r17FSRXwKS0xXa6//5ds/kSETITMW5T6LoFDbshNvU5O3Yh170uzQyhZ4eb6Q0
gHN64kyNAODtiW1/PmejPuOz5rpo/5lE2ANiIYs1vj/PJY2EP350JGzLRwGEFIgl
W41lYtOdob0351slFmVn3eTXZo7GYG03DQZgQAp9NlaGhgA1kXfbOH/9J6mZsGh9
ufpjB92w3ueT9f6ZJ71/sh0kABHxZGuW+XycCwBoM6aG00OwIRV160uSwLt/24om
XVz2VOcIZSM5iC+JFwS2kUbdDq+pah9eAELNIzYaa02+UyVjQAPIMUmg1M0F4Pct
Xpkf3yqfxoG6UyfV3zD3oPWW3O1Y5+miGzCnoVEHDQ0kV1eYBvGDiO+HJjqRqvis
6ExmF1laqyumMECUtYfAyjlCpVdHHLGPfpiB7Ygc97Jccb8oOGJX2sFv1cf12lZX
yr7hup8LDqZEV7ZNoGLjP+1MD+KDc4i4WPReziRgTvvh9cuPLrBtUlq9YtKGGasW
zCyONrub5j/Pn4EO/Sd4T7FX67D7N/gU6bk/PEEcCAzoXqQ2P40S5HoikVgUByNA
dRkwv0sNrNewvgq7KL/TjdQCiLVEAhvHviM5XglD2TtLlKYw6EmQXDlkvvcNJjWP
Ng9ZI+RvjDdSi/XsrA==
-----END CERTIFICATE-----
)PEM";

static const char *PROV_CLIENT_CERT_PEM = R"PEM(
-----BEGIN CERTIFICATE-----
MIIExzCCAq+gAwIBAgIUH+oDkTA80WkVCEXkMp/5ofQPHsIwDQYJKoZIhvcNAQEL
BQAwZDELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAkNPMRAwDgYDVQQHDAdCb3VsZGVy
MQ0wCwYDVQQKDAROSVNUMQwwCgYDVQQLDANVQVMxGTAXBgNVBAMMEFVBUzYtSW50
ZXJuYWwtQ0EwHhcNMjYwMjA2MTYxOTI3WhcNMjgwNTExMTYxOTI3WjBmMQswCQYD
VQQGEwJVUzELMAkGA1UECAwCQ08xEDAOBgNVBAcMB0JvdWxkZXIxEjAQBgNVBAoM
CU5JU1QtUFNDUjEQMA4GA1UECwwHRm94Tm9kZTESMBAGA1UEAwwJRm94Tm9kZTE0
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArfuFZYbFvr7JM+0SQ6XP
AxLy7wYWcTGtT6oOhqMys1Wl6bP2E/C8uiPG3Bbckf3vyK6KRe0FAed0N0zvnlln
GONRfxkIm90U5BVAKnrPqOTVVeAReVIbFWM2qBJWC8zdUGYFmudJlJUm++okHxJF
4lt91MjOUHTA9krFW0hK6kCE0sgfYMd+C8xGyTS6UfwvwrBxnAlNSeyBlPf1gwMX
icQMbNo5+G7canl10O4LMtBsroBJQDWPKttavOlQiLPD0WIgwhJsmxxots5hM5HL
DqmEwfWKuAAo4lgAnxdnyyFcYLXR/ilKvHPyz5B7bD9x8vnbwbT7cy7lSea/BuNH
UwIDAQABo28wbTAfBgNVHSMEGDAWgBTFJSf/FJbc/iMKB6cftpCJS6DYbDAJBgNV
HRMEAjAAMAsGA1UdDwQEAwIFoDATBgNVHSUEDDAKBggrBgEFBQcDAjAdBgNVHQ4E
FgQUhnPjle4vLxKQZJDkcigw5RIxmWgwDQYJKoZIhvcNAQELBQADggIBAI6Ve5MR
AAGDgtNRVYppDD8Z2HgFWjze7LSeOg/Enb5QssK909yUZ0GXwa6IVTR8Xc6M5Xuc
LdpUJ3d2/h3tJvW3YX5hpZvLIDZ6Dxvhwg0+H1AHMG+O5FVSZd+rFYe+aH7THur5
jBg6NQlo2DR0VpIyd3dmNRQZG1WN4R0H4LDCQGibLyANtcS3vc7YG27+DNrU9nf/
u26L7J7oAqLwYJ6v8JSfvWIkwZCOgvlF+1XtT+WkaYlnE/WL53+rVR4tL7bFrPxJ
EX78jS5UIKO3yK1hGeqscSvC+etg9UTCLPB2ei6kxR36eYmNVPELQ+//uT5+a7RC
iQ8BRD/EawC5Ggt/CuPZReC5izLwp23doHt8bbidYhvlglj+0kDu3c2zmpkD6Vwf
RoIAgwVvanbh9Y6A4EGnMkIJtYGnDght8OLVw4Lvt7WVKcsEfewyWI36KCGA0CQr
pvyYRldwemnEwWI+8rTS7zjJlhfx7mKMCyrB4fnMZmPDxjKzVEOEimJiBdcQJ2ox
I5NQ2wbmHWaWUId0fTsA31hRqahWI/F2u5wDb2/YN2t4gU1ddeCklI32a5Z0cl4s
Ok8+sIqUCbymg+eI9XD7l+3ViMPwTFTM+j9WYiPqMYW5iTk6J+SWqyKOZn2iMcQ6
ACQaThUsjsKexf5zS/jgbRrIfx86jdjt4jUU
-----END CERTIFICATE-----
)PEM";

static const char *PROV_CLIENT_KEY_PEM = R"PEM(
-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCt+4VlhsW+vskz
7RJDpc8DEvLvBhZxMa1Pqg6GozKzVaXps/YT8Ly6I8bcFtyR/e/IropF7QUB53Q3
TO+eWWcY41F/GQib3RTkFUAqes+o5NVV4BF5UhsVYzaoElYLzN1QZgWa50mUlSb7
6iQfEkXiW33UyM5QdMD2SsVbSErqQITSyB9gx34LzEbJNLpR/C/CsHGcCU1J7IGU
9/WDAxeJxAxs2jn4btxqeXXQ7gsy0GyugElANY8q21q86VCIs8PRYiDCEmybHGi2
zmEzkcsOqYTB9Yq4ACjiWACfF2fLIVxgtdH+KUq8c/LPkHtsP3Hy+dvBtPtzLuVJ
5r8G40dTAgMBAAECggEABa2u5ehnLdmWQY6cSP7xab1Flo2RT/J6IJ6BSN2suvOx
rZYFExOJZV3jkK1iV5V9LQSdWLjqr+w4+9EUBQwP9Fz+/PH5OPUXSHZ1GK3Xd0C4
wcvFZjtKzQDm+MdYbZwrIQOjbTHdUe8oTZKPyT/UbhXkXnyLV07oOWGpXVhXsZuJ
retaJGR57/4tZb4VqCFXsk7aFSE70REW9jw82w1z8IA5rc3Iwg4j7Jj5LX1B5ZoU
rDrbCvVwfRQNYRXAOEsRnjpc6TFlNyN6ys1KZjMi0ZdECpmblL0z8Y0I3r0N+5Ep
et+qJPo/HQ2KZyvpzEfhnDj1EvxFM35jQOCzrid18QKBgQDfR8aELlDN3PK0hmHk
NbpwgpYU7xRtYdvKMhYVe4VjjatK5Zt8mmwcnNHqW1blAXKGiunkMrKCpl0zBLi6
ettPMyEk2GHJsY3oLJXhHmpNC2/oLBnq23BD4JsFGXUpW2dXmUtuM/+qH7ndDXAs
yH1wqKEUcF6vbxMs08EvnhXF+wKBgQDHel3fr5O7Qd2YA+7LUDLtXq0LpyyIgKvS
J/mPsBYPMCOBfrE1TtLCWRir1nsY7MqoOeegUVBerVv9sdTA5R567QHGd9XtplTx
fHKFpiUYrK9NnHt/gYKHNJsP5SYqu/JiIHEW/yKOJTS0Xxx7I2CYy6e7tyU0d4fx
hKIzwAi8iQKBgDTVszAMFarhIb1+HyP1YszE2ebSPC81/OB3waoLtKC+IU5zzrtJ
f1O1+CZQvtsp4IPd00LgjEVGQL+V2nCmKccv/iPN4DXuAfpysnS20cixcCWsaeWd
T4+AUq5+O68xpd6gs6y8OT90inK4f1bZinViMdu4S2+QBSWSZiBydAyJAoGAVzJ+
ZpFxmqGoJBNUBzv/hxOjwTxKk0Dr2unuhYkLm4gHfUvMWukh6CLaTgNru/GGhRMK
1WG2KCzP0Y4y2j9Nm56O8BriQQg8iXNE/HcsMCUMXHjS42HRlRjBz6jzjiqqQYq8
5LiSemB5MX+CC7HjP+eGYuAkO1Mk8cZwwP5o00ECgYEAmnWKPrHm6nXvUE+Uun7d
L2pvKVp2nZzEHeV6vYGqy3tyNqqVAteYeDLfNnLUxryZ8yCMJcncg1MWI3YmZ0Ky
ymNXWA8Hfi6AOZ74FHsqacB5SBLVw9PKalYpTqzb3VA2scu9CsiZxyHMGOea7+3z
sxCmo59dRbcmMrVmOjSGyaw=
-----END PRIVATE KEY-----
)PEM";

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("FoxNode Provision Build: writing secrets to NVS");

  if (!secretsInit()) {
    Serial.println("ERROR: secretsInit failed");
    while (true) delay(1000);
  }

  // Guard: do not overwrite if already provisioned
  if (secretsHasWiFi(WifiProfile::NTP) && secretsHasWiFi(WifiProfile::UAS6) && secretsHasMtls()) {
    Serial.println("Secrets already provisioned. Not overwriting.");
    Serial.println("If you want to rotate secrets, call secretsClearAll() then reflash.");
    while (true) delay(1000);
  }

  bool ok1 = secretsSetWiFi(WifiProfile::NTP,  PROV_NTP_SSID,  PROV_NTP_PSK);
  bool ok2 = secretsSetWiFi(WifiProfile::UAS6, PROV_UAS6_SSID, PROV_UAS6_PSK);

  // Sanitize PEM to remove CRLF / stray characters that can break mbedTLS parsing.
  String ca  = sanitizePem(PROV_CA_PEM);
  String crt = sanitizePem(PROV_CLIENT_CERT_PEM);
  String key = sanitizePem(PROV_CLIENT_KEY_PEM);

  bool ok3 = secretsSetMtls(ca, crt, key);

  Serial.printf("WiFi NTP write:  %s\n", ok1 ? "OK" : "FAIL");
  Serial.printf("WiFi UAS6 write: %s\n", ok2 ? "OK" : "FAIL");
  Serial.printf("mTLS write:      %s\n", ok3 ? "OK" : "FAIL");

  if (ok1 && ok2 && ok3) {
    Serial.println("Provisioning complete. Next: flash the normal FoxNode firmware.");
  } else {
    Serial.println("Provisioning failed. Check NVS capacity and PEM sizes.");
  }

  while (true) delay(1000);
}

void loop() {}