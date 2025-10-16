#pragma once
#include <cstdint>
#include <string>


// Defienes the maximum size of the metadata in bytes
// This sets how many bytes will the code be able to process as a report
#define MAX_STRUCTURE_SIZE 512
#define LORA

#define PACKET_SIZE 502


#define REPORT 0
#define STRUCT_CONF 1

class Comm {
public:
    Comm();

    ~Comm();
    
    template <typename T>
    int addField(std::string field, int maxLength = 0);
    /*
        Adds a field of type T, with the give fieldname.
        Strings should use the std::string type, and specification of max string length is neccesary 
    */
    
    template <typename T>
    int setField(std::string field, T value);
    /*
        Sets the given field's value to "value". Type should not differ from what was set for this field with addField()
    */

    template <typename T>
    T getField(std::string field);
    /*
        Returns the value of the given field.
    */

    int sendReport();
    /* Send teh value of all datafields */

    int sendStructure();
    /* send packet metadata, based on curently existing fields */

    int processRawData(uint8_t* data, int dataLength);
    /* Processes the data that have been received */

    void receiverCallback(int len);
private:
    int send(uint8_t* data, int dataLength);
    int sendData(uint8_t* data, int dataLength, uint8_t packetType);
    int handlePacket(uint8_t* data, int size);
    
    int buffSize = PACKET_SIZE;
    int dataSize = PACKET_SIZE;

    uint8_t* outBuff = (uint8_t*) std::malloc(buffSize);
    uint8_t* inBuff = (uint8_t*) std::malloc(buffSize);
    uint8_t* lastPacket = (uint8_t*) std::malloc(dataSize);

    std::string structure[MAX_STRUCTURE_SIZE];
    int8_t continuation = -1;
    uint16_t packetSize;

    int inSeqNum = 0;
    int outSeqNum = 0;
};
