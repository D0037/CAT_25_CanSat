#include <comm.hpp>
#include <string>
#include <LoRa.h>

int main() {
    // Initialize lora
    if (!LoRa.begin(868E6))
    {   
        // IDK should print some error ig
        return -1;
    }

    // Create interface
    Comm comm();

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

    // On the receiver side add a packet receive callback:
    LoRa.onReceive(comm.receiverCallback);

    // you read can field values like this (types should match)
    int example_int = comm.getField<int>("example_int");
    
    // For strings:
    std::string test = comm.getField<std::string>("string_example");
}