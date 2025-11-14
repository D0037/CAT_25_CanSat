#pragma once
#include <cstdint>
#include <string>
#include <vector>


// Defienes the maximum size of the metadata in bytes
// This sets how many bytes will the code be able to process as a report
#define MAX_STRUCTURE_SIZE 512
#define LORA

#define PACKET_SIZE 502

#define REPORT 0
#define STRUCT_CONF 1

class Comm {
public:
    /* Constructor, a hardware transmit function should be supplied that has 2 arguments: `uin8_t* buffer`, and `int size`*/
    Comm(int (*f)(uint8_t* data, int size));

    /* Destructor */
    ~Comm();

    /*
        @brief Adds a field of type `T`, with the give fieldname.

        Strings should use the `std::string` type, and specification of max string length is neccesary
        @tparam T type of the field
        @param field name of the field
        @param maxLength the maximum allowed length for strings
    */
    template <typename T>
    int addField(std::string field, int maxLength = 0);

    
    /*
        @brief Sets the given field's value.
        Type should not differ from what was set for this field with `addField()`

        @tparam T type of the field
        @param field name of the field
        @param value the value to set
    */
    template <typename T>
    int setField(std::string field, T value);


    /*
        @brief Returns the value of a given field

        @tparam T the type of the field. is neccessary, unless the compiler specifically knows it from the expected return value
        @param field the name of the field

        @returns the value of the field
    */
    template <typename T>
    T getField(std::string field);

    
    /*
        @brief Transmits all the fields' values.
    */
    int sendReport();


    /*
        @brief send packet metadata based on currently existing fields. if fields have been added should be ran again
    */
    int sendStructure();


    /*
        @brief this is a function that should be called when a packet arrives
    */
    void receiverCallback(uint8_t* data, int len); 


    /*
        @brief tells whether a sync packet has arrived or not

        @returns bool
    */
    bool getSynced();


    /*
        @brief checks if new report packets have arrived since the last call of this function

        @return bool
    */
   bool isUpdated();
private:
    int send(uint8_t* data, int dataLength);
    int sendData(uint8_t* data, int dataLength, uint8_t packetType); // sends dataLength bytes of data, handles headers
    int handlePacket(uint8_t* data, int size, int type); // handles packets, that have already been preprocessed, and stripped of headers
    int processRawData(uint8_t* data, int dataLength); // Processes the data that was received
    int (*writeHAL)(uint8_t*, int); /* communication transmit hardware abstraction layer, set by constructor
    it is only required to deal with a maximum packet size of 255 bytes*/
    
    
    int buffSize = PACKET_SIZE;
    int dataSize = PACKET_SIZE;

    uint8_t* outBuff = (uint8_t*) std::malloc(buffSize);
    uint8_t* inBuff = (uint8_t*) std::malloc(buffSize);
    uint8_t* lastPacket = (uint8_t*) std::malloc(dataSize);

    std::vector<std::string> structure;
    int8_t continuation = -1;
    uint16_t packetSize;

    int inSeqNum = 0;
    int outSeqNum = 0;

    bool synced = false;
    bool updated = false;
};

#include "comm.cpp"