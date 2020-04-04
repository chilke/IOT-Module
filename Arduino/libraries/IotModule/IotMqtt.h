#ifndef IOT_MQTT_H
#define IOT_MQTT_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#define MQTT_CONNECT_RETRY 5000
#define MQTT_RESPONSE_TIMEOUT 10000

#define MQTT_CA_CERT_FILE "/ca.crt"
#define MQTT_CLIENT_CERT_FILE "/client.crt"
#define MQTT_CLIENT_KEY_FILE "/client.key"

#define MQTT_CA_CERT_BAK_FILE "/ca.crt.bak"
#define MQTT_CLIENT_CERT_BAK_FILE "/client.crt.bak"
#define MQTT_CLIENT_KEY_BAK_FILE "/client.key.bak"

class IotMqtt {
public:
    void init();
    void handle();

    bool backupCerts();
    bool restoreCerts();
private:
    bool loadCerts();
    void connect(uint32_t m);
    void sendDeviceInfo();

    String hostname;
    int port;
    char topicBuf[20];
    BearSSL::X509List cert;
    BearSSL::X509List client_crt;
    BearSSL::PrivateKey key;
    WiFiClientSecure net;

    bool loaded;
    uint32_t lastConnectAttempt;

    PubSubClient client;
};

extern IotMqtt Mqtt;

// Obtain First CA certificate for Amazon AWS
// https://docs.aws.amazon.com/iot/latest/developerguide/managing-device-certs.html#server-authentication
// Copy contents from CA certificate here ▼
static const char cacert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

// Copy contents from XXXXXXXX-certificate.pem.crt here ▼
static const char client_cert[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
MIIDWjCCAkKgAwIBAgIVAKpBIek4giA8T7tUzz8JMmCi6+J2MA0GCSqGSIb3DQEB
CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t
IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0yMDAzMDcxNzUz
MjdaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh
dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCsQMZhj8H43gkh1+P+
vNMyoBZimcGfC9Oh7IlAq/PQieunVB8tkQa6QADPHuuzr334DmkifzapdFY4spMJ
mzapqO2uje5pbIXY24093ejl0MSgsFAfH/2/20I+AOT+nWRCU9NPQBVOr8kkW2WH
ZbI0BxF3EBVrcmrQlF/m63KUKpcVNX5qFbRBuxu3CKcmlvy5uVDUqUpqmPfLTgEy
iDj4n5ISUVsLtXxrLVFv/MxvJRttU+MM2KWBhnEZoImftBK4tD5MHdIh6g3me15Q
AlVJqnOlk9HWAItK1wfdyrX4gzQaNSaQal+wiWvhXLE1RD34MPZk6nC598gQ/cXR
ik1nAgMBAAGjYDBeMB8GA1UdIwQYMBaAFF7djJywtGzUfwKwqN9KaRykultwMB0G
A1UdDgQWBBTNOTeplw23ol8Lxhex6JAzXwvIfDAMBgNVHRMBAf8EAjAAMA4GA1Ud
DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAaD0LoFcVx4qrvSpMiY7X+3w4
6mnnj/nTVWWtzftHzsZOR3UXMHTG1z7JVfBTbPj21vX1ffHLUbGw0WDDaJuJpSj6
CDRAmMBlJI9atrk6adIUZjdmfBo9f3DloedT/QUrWbUK61gQdRAsAsb0PpA6841z
rlbBFcw5iv4CRMKAql0o3FVS1gW2pdZpfUjOOzcpVpqUumcx7FuyGVfggQFFDISQ
A6uhdUaJVXw9wUnYTD6TlMIb/hXSaCx5mTnFEBW+O9WDITj5YN3gIMQhJZEr6Tg9
nU2AmAyFs5My95FQ4BRrvIJKZcj4u+jhAXnKq63ncNkcVOBQDved8DBIcU+YsQ==
-----END CERTIFICATE-----
)KEY";

// Copy contents from  XXXXXXXX-private.pem.key here ▼
static const char privkey[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEArEDGYY/B+N4JIdfj/rzTMqAWYpnBnwvToeyJQKvz0Inrp1Qf
LZEGukAAzx7rs699+A5pIn82qXRWOLKTCZs2qajtro3uaWyF2NuNPd3o5dDEoLBQ
Hx/9v9tCPgDk/p1kQlPTT0AVTq/JJFtlh2WyNAcRdxAVa3Jq0JRf5utylCqXFTV+
ahW0QbsbtwinJpb8ublQ1KlKapj3y04BMog4+J+SElFbC7V8ay1Rb/zMbyUbbVPj
DNilgYZxGaCJn7QSuLQ+TB3SIeoN5nteUAJVSapzpZPR1gCLStcH3cq1+IM0GjUm
kGpfsIlr4VyxNUQ9+DD2ZOpwuffIEP3F0YpNZwIDAQABAoIBAF9p6fl0tg9yfkB+
483hLmOmhX/Mh5Hf7TSOX2CxXvauzxf64uJMqRtWBJy2Ff2MQgy7fsUbDl5DENeL
IyN5FIhaCH5eJCfFc0ayJsSUPUO/posT3u2+vMIfY8wQlsfqyvRqlmwZpTPW72Xy
MoUTceib2bLH9/VyhXRQG+c/mFI1dDuZpl5S7vW9mQjaxwhLx3HYKS/+NwwK9uns
PiF8X/RE22bAH/1kMMK1GUsqvCnFI4jiOapJA3FMQAjC2JTqR1lkC413v5f8CNWd
Vlfqb6r9tIzeuNbBxARxze7UNp1x9nhvQ4lvONvP/sUFddXfXt3mbvXxuDP3d3iI
5hp8thECgYEA3ry10jeYAFu2zkmzT9y00qXkLKG+d9s30m2kC+mO6E+zIzhP3CAS
stsp/CM+Kcz9J+L7BQjjtadvOU9C0ltEUk8SUoiJlx4/pDxyYqpUE87yVWu/aBJb
90eT+lmHLDfEVGCDuL1hrLlAeVX0NUxICK0V5WuCMWzrDrqzGLKJpfMCgYEAxfoL
skpRRDDZI5d6JyiUEqdOSWPKoUsWmZ48zsFOCot4CSpJ4ssq3FtSYrXyDHrMBuep
B6y/X0jhCRI7voS3UB6h2AICAgIaTq+XP9hHrlZdH3aGI3W+JKH5dLl9raZWyaZi
/1RVTzdBFmO31zWaa6tJAsE9nDEHDuQwQVCvU70CgYEAv5Fyl+D0vKok6ft9W1yp
sNq5CqH6HT/QHQ3rn8s6/BdALTSLivmS232f+JmfJ3iE6z9PQh9JKb1d7d57zExu
0JPtYErKe8yqU+khQDbapUbCKjHHqAzSSDWp0hIaLi+wjhmMcOGBFnsmNrV0mjqH
Yd7sDCuRCE1zRXHSA5XayX0CgYAOfzviB10iEC2mVI5GVctZ/lRWPW92FiiGgTbU
sQUE8qeMjEMyz0vJLQrAXbBmx4svKkGPZU/yK0UHRAWJ8u/1dvjE94isGvlQL710
mGLPWlWhWy2BZTrSNeYtbPPOBnMuevee2M8SaOpkOFMa+DrhjX5gLERVrqVkqsL5
1W2xRQKBgQCNL6yUujEfUtjKOxBozPq4FnqIwarkmulQGxribmVRoOgGy8kqQ0Fw
Q5FCtLO/rJTX6ja6hUB/jabUx6iEb2f5+muFXQidMcdiVbiap6ezCXBYCV/tPoQE
Oi426a6TzejUpY2dC1eXXgN20vSCIgj4j/J03AsAU/cd6NiHXoOapw==
-----END RSA PRIVATE KEY-----
)KEY";

#endif