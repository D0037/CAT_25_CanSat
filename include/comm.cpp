#include <string>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <type_traits>
#include "comm.hpp"
#include <vector>
#include <LoRa.h>

template <typename T>
int Comm::addField(std::string field, int maxLength)
{
    // handle std::string differently
    if constexpr (std::is_same_v<T, std::string>)
    {
        // Iterate through fields, also checks if new field fits
        for (int i = 0; i < PACKET_SIZE - maxLength + 1; i++)
        {
            if (field == structure[i]) return -2;   // Exit if field already exists
            
            // Empty field found
            if (structure[i] == "")
            {
                // Create maxLength number of fields for the string
                for (int j = 0; j < maxLength; j++)
                {
                    structure[i + j] = field;
                }

                return 0;
            }
        }

        return -1;
    }
    else // Handles every other datatype
    {
        // Iterate through fields: checks for field repetition, also check if new field fits
        for (int i = 0; i < PACKET_SIZE - sizeof(T) + 1; i++)
        {
            if (field == structure[i]) return -2; // Exit if field name already exists
            
            // create fields for the data
            if (structure[i] == "")
            {
                for (int j = 0; j < sizeof(T); j++)
                {
                    structure[i + j] = field;
                }

                return 0;
            }
        
        }

        return -1;
    }
}

template <typename T>
int Comm::setField(std::string field, T value)
{
    // handle strings
    if constexpr (std::is_same_v<T, std::string>)
    {
        for (int i = 0; i < PACKET_SIZE; i++)
        {
            if (field == structure[i])
            {
                // check if the string to store isn't too long
                if (i + value.length() > PACKET_SIZE-1 || structure[i + value.length() - 1] != field) return -2;
                // store data in the output buffer
                std::memcpy(outBuff + i,  value.c_str(), value.length());
                return 0;
            }
        }
        return -1;
    }
    else // every other datatype
    {
        for (int i = 0; i < PACKET_SIZE; i++)
        {
            if (field == structure[i])
            {
                std::memcpy(outBuff + i, &value, sizeof(T));
                return 0;
            }
        }
        return -1;
    }
}

template <typename T> // type of field to get
T Comm::getField(std::string field)
{
    T value;
    if constexpr (std::is_same_v<T, std::string>)
    {
        // store strings character by character
        for (int i = 0; i < PACKET_SIZE; i++)
        {
            if (structure[i] == field)
            {
                value += lastPacket[i];
            }
        }
    }
    else
    {
        for (int i = 0; i < PACKET_SIZE; i++)
        {
            if (structure[i] == field)
            {   
                // casts to correct pointer type, dereferences and returns the value
                return * (T*)(lastPacket + i);
            }
        }
    }

    return value;
}

int Comm::sendData(uint8_t* data, int dataLength, uint8_t packetType) {
    std::cout << std::hex << std::uppercase;

    // increase buffer size, if it is needed
    if (dataLength > buffSize)
    {
        inBuff = (uint8_t*) std::realloc(inBuff, dataLength);
        outBuff = (uint8_t*) std::realloc(outBuff, dataLength);
    }

    // creates 251 byte long transfer packets and adds headers
    for (int i = 0; i < (int) (dataLength / 251); i++)
    {
        uint8_t sendBuff[255] = { 0 };

        // 1 byte Seqence number
        sendBuff[0] = outSeqNum++ % 256;

        // upper half: transfer packet continuation, lower half: packet type
        sendBuff[1] = packetType | ((i != 0) << 4);

        // low byte of packet size (only relevant in the leading transfer packet)
        sendBuff[2] = dataLength & 0x00FF;

        // which packet is this a continuation of, high byte of packet size in the leading transfer packet
        sendBuff[3] = i == 0 ? ((uint16_t) dataLength & 0xFF00) >> 8 : (outSeqNum - i - 1) % 256;

        // send data
        std::memcpy(sendBuff + 4, data + (i * 251), 251);
        send(sendBuff, 255);
    }
    // Deals with remaining data
    if (dataLength % 251 != 0)
    {
        uint8_t sendBuff[4 + (dataLength % 251)]; // creates correctly sized send buffer
        sendBuff[0] = outSeqNum++; // Sequence number
        sendBuff[1] = 0x10 | packetType; // continuation and type of packet
        sendBuff[3] = (outSeqNum - (int) (dataLength / 251) - 1) % 256; // Which packet is this a continuation of

        // Sends data
        std::memcpy(sendBuff + 4, data + dataLength - (dataLength % 251), dataLength % 251);
        send(sendBuff, 4 + (dataLength % 251));
    }

    return 0;
}

int Comm::processRawData(uint8_t* data, int dataLength)
{
    uint8_t header[4];
    std::memcpy(header, data, 4);

    bool continuation = header[1] & 0x10;

    // Only process packet if sequence number is correct
    // And also when tranfer packets have been lost, but the current first part of a data packet, in which case the data can be parsed
    if (header[0] == inSeqNum % 256 || !continuation)
    {   
        // First transfer packet of larger data packet
        if (!continuation)
        {
            packetSize = header[2] | (header[3] << 8);

            // Buffer is not large enough, resize is needed
            if (packetSize > buffSize)
            {
                inBuff = (uint8_t*) std::realloc(inBuff, packetSize);
                outBuff = (uint8_t*) std::realloc(outBuff, packetSize);

            }

            // copy to buffer
            std::memcpy(inBuff, data + 4, std::min<uint16_t>(packetSize, 251));
            
            // packets with no continuation
            if (packetSize < 251) handlePacket(inBuff, packetSize);
        }
        else
        {
            uint8_t packetDiff = header[0] - header[3]; // Difference in sequence number between the first and the latest received packet of larger data
            uint8_t* dest = inBuff + (251 * (int) packetDiff); // Computes correct pointer to copy received data to
            std::memcpy(dest, data + 4, dataLength - 4);
            int dataReceived = (int) (251 * (int) packetDiff);
            dataReceived += dataLength - 4; // Keep track of how much data has been received in this larger data packet. Used to calculate how much more data to expect

            // packet is over, now it can be processed further
             if (packetSize == dataReceived) handlePacket(inBuff, packetSize);

        }
        // update sequence number, uses the packet's seqNum to correct itself if packets have been lost, but data becomes parsable again
        inSeqNum = header[0] + 1;

        return 0;
    }
    return -1;
}

int Comm::handlePacket(uint8_t* data, int size)
{
    uint8_t packetType = *(data + 1) & 0x0F;

    switch (packetType)
    {
        case REPORT:
            std::memcpy(lastPacket, data, size);
            break;

        case STRUCT_CONF:
            int fieldCounter = 0;
            for (int i = 0; i < size; i++)
            {
                if (data[i] == 0x00) // Null termination
                {
                    fieldCounter++;
                    continue;
                }

                // Add character to current processed field
                structure[i] += (char) data[i];
            }

            break;
    };
}

int Comm::send(uint8_t* data, int dataLength) {
    if (dataLength > 255) return -1;

    #ifdef LORA
    LoRa.beginPacket();
    LoRa.write(buffer, len);
    LoRa.endPacket();
    #endif

    return 0;
}

int Comm::sendStructure()
{   
    int dataSize = 0;
    // Calculate the size of the buffer (+1 is for null)
    for (std::string field : structure) dataSize += field.length() + 1;
    
    // Allocate memory for the buffer
    char* data = (char*) std::malloc(dataSize);

    // Iterate through fields, copying the fieldname
    // Separation is marked by null termination of each string
    int i = 0;
    for (std::string field : structure)
    {
        // copy stringdata, and update index
        std::strcpy(data + i, field.c_str());
        i += field.length() + 1;
    }

    // send
    sendData((uint8_t*) data, dataSize, STRUCT_CONF);
    std::free(data);
}

void receiverCallback(int len)
{
    #ifdef LORA
    for (int i = 0; i < len; i++)
    {
        int data = LoRa.read();
        while (data == -1);
        inBuff[i] = (uint8_t) data;
    }
    #endif
}

Comm::Comm()
{}

Comm::~Comm()
{
    std::free(inBuff);
    std::free(outBuff);
}

int main() 
{
    Comm comm;

    comm.sendStructure();

    return 0;
}