#include <LoRa.h>

void setup()
{
    Serial.begin(115200);
    while(!Serial);
    
    if (!LoRa.begin(915E6)) {
        Serial.println("failed to initialize LoRa.");
        while (1);
    }
    

}
void loop() {}

void callback(int size) {
    String packetHex = "";

    for (int i = 0; i < size; i++)
    {
        packetHex += String((char) LoRa.read(), HEX)
    }

    Serial.println("received packet:" + packetHex);
}