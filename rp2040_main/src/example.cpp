#include <comm.hpp>
#include <string>
#include <LoRa.h>


// you should supply a function that can send a packet to the receiver
// the max possible packet size is 255 bytes
// the arguments should be the buffer and the size of it
void send(uint8_t* data, int size) {
    LoRa.beginPacket();
    LoRa.write(data, size)
    LoRa.endPacket();
}


int main() {
    // Initialize lora
    if (!LoRa.begin(868E6))
    {   
        // IDK should print some error ig
        return -1;
    }

    // Create interface and set transmit function
    Comm comm(send);

    // Create fields
    comm.addField<int>("example_int");
    comm.addField<unsigned long long>("example_ull"); // Any primitive can be used basically
    comm.addField<std::string>("string_example", 8); // Please use std::string for string types. It is also necessary to set a maximum length

    // Sends packet metadata to the receiver
    comm.sendStructure();

    // Set field values
    comm.setField("example_int", 16);
    comm.setField("example_ull", (unsigned long long)42069); // Always make sure that it is specifically the type that has been set as the field type

    // Sends field values
    comm.sendReport();
}