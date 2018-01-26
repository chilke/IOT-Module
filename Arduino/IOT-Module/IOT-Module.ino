#include <UDPListener.h>
#include <WifiConnectionManager.h>

void setup() {
  // Start Serial
  Serial.begin(115200);

  // Setup PINs
  pinMode(4, INPUT_PULLUP);
  // Check for safemode startup
  uint8_t count = 0;
  while(digitalRead(4) == 0) {
    Serial.print(".");
    if (++count > 50) {
        setupConfigAP();
    }
    delay(100);
  } // end check for safemode startup

  // Setup Over the Air update handler
  udpBegin();
}

void loop() {
  // Handle connection manager events
  wcmHandle();
  // Handle Over the Air update events
  udpHandle();
}
