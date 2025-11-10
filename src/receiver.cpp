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

    // this will be used as the callback when a packet arrives
    void callback(int size) {
        uint8_t* inBuff = std::malloc(size); // allocate size bytes
        LoRa.readBytes(inBuff, size);   // read size bytes into buffer
        
        comm.receiverCallback(inBuff, size); // call callback of library

        std::free(inBuff); // free buffer
    }
    
    // register the callback
    LoRa.onReceive(comm.receiverCallback);

    // put lora in receive mode
    LoRa.receive()

    // you read can field values like this (types should match)
    int example_int = comm.getField<int>("example_int");
    
    // For strings:
    std::string test = comm.getField<std::string>("string_example");
}