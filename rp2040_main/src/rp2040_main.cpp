#include <comm.hpp>
#include <string>
#include <LoRa-RP2040.h>
#include "pico/stdlib.h"

// you should supply a function that can send a packet to the receiver
// the max possible packet size is 255 bytes
// the arguments should be the buffer and the size of it
int send(uint8_t* data, int size) {
    LoRa.beginPacket();
    LoRa.write(data, size);
    LoRa.endPacket();

    return 0;
}


int main() {
    stdio_init_all();
    printf("starting...\n")3;

    printf("initializing comms interface...\n");
    // Create interface and set transmit function
    Comm comm(send);
    
    printf("Initializing LoRa...\n");
    // Initialize lora
    while (!LoRa.begin(868E6))
    {   
        printf("LoRa initialization failed, retrying...\n");
        sleep_ms(1000);
    }


    // Create fields
    comm.addField<int>("example_int");
    comm.addField<unsigned long long>("example_ull"); // Any primitive can be used basically
    comm.addField<std::string>("string_example", 8); // Please use std::string for string types. It is also necessary to set a maximum length

    printf("sending packet structure...  \n");
    // Sends packet metadata to the receiver
    comm.sendStructure();

    // Set field values
    comm.setField("example_int", 16);
    comm.setField("example_ull", (unsigned long long)42069); // Always make sure that it is specifically the type that has been set as the field type

    printf("transmitting data... \n");
    // Sends field values
    comm.sendReport();
}