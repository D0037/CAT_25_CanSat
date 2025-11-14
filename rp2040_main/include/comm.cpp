#include <string>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <type_traits>
#include "comm.hpp"
#include <vector>

template <typename T>
int Comm::addField(std::string field, int maxLength)
{
    // check if a field alread exists with the name
    for (std::string f : structure) if (f == field) return -2;

    // handle std::string differently
    if constexpr (std::is_same_v<T, std::string>)
    {

        // Add new fields
        for (int i = 0; i < maxLength; i++)
        {
            structure.push_back(field);
        }

        return 0;
    }
    else // Handles every other datatype
    {
        // Add new fields
        for (int i = 0; i < sizeof(T); i++)
        {
            structure.push_back(field);
        }

        return 0;
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
                if (i + value.length() > structure.size() || structure[i + value.length() - 1] != field) return -2;
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
        for (int i = 0; i < structure.size(); i++)
        {
            if (structure[i] == field)
            {
                value += lastPacket[i];
            }
        }
    }
    else
    {
        for (int i = 0; i < structure.size(); i++)
        {
            if (structure[i] == field)
            {   
                // casts to correct pointer type, dereferences and returns the value
                return *(T*)(lastPacket + i);
            }
        }
    }

    return value;
}

int Comm::sendReport() {
    return sendData(outBuff, structure.size(), REPORT);
}

int Comm::sendData(uint8_t* data, int dataLength, uint8_t packetType) {

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
        sendBuff[1] = ((i != 0) << 4) | packetType;

        // low byte of packet size (only relevant in the leading transfer packet)
        sendBuff[2] = dataLength & 0x00FF;

        // which packet is this a continuation of, high byte of packet size in the leading transfer packet
        sendBuff[3] = i == 0 ? ((uint16_t) dataLength & 0xFF00) >> 8 : (outSeqNum - i - 1) % 256;

        // send data
        std::memcpy(sendBuff + 4, data + (i * 251), 251);
        writeHAL(sendBuff, 255);
    }
    // Deals with remaining data
    if (dataLength < 251) {
        uint8_t sendBuff[4 + dataLength];
        sendBuff[0] = outSeqNum++;
        sendBuff[1] = packetType;
        sendBuff[2] = dataLength;
        sendBuff[3] = 0;

        std::memcpy(sendBuff + 4, data, dataLength + 4);
        writeHAL(sendBuff, dataLength + 4);
    }
    else if (dataLength % 251 != 0)
    {
        uint8_t sendBuff[4 + (dataLength % 251)]; // creates correctly sized send buffer
        sendBuff[0] = outSeqNum++; // Sequence number
        sendBuff[1] = 0x10 | packetType; // continuation and type of packet
        sendBuff[3] = (outSeqNum - (int) (dataLength / 251) - 1) % 256; // Which packet is this a continuation of

        // Sends data
        std::memcpy(sendBuff + 4, data + dataLength - (dataLength % 251), dataLength % 251);
        writeHAL(sendBuff, 4 + (dataLength % 251));
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
            if (packetSize < 251) handlePacket(inBuff, packetSize, header[1] & 0x0F);
        }
        else
        {
            uint8_t packetDiff = header[0] - header[3]; // Difference in sequence number between the first and the latest received packet of larger data
            uint8_t* dest = inBuff + (251 * (int) packetDiff); // Computes correct pointer to copy received data to
            std::memcpy(dest, data + 4, dataLength - 4);
            int dataReceived = (int) (251 * (int) packetDiff);
            dataReceived += dataLength - 4; // Keep track of how much data has been received in this larger data packet. Used to calculate how much more data to expect

            // packet is over, now it can be processed further
            if (packetSize == dataReceived) handlePacket(inBuff, packetSize, header[1] & 0x0F);

        }
        // update sequence number, uses the packet's seqNum to correct itself if packets have been lost, but data becomes parsable again
        inSeqNum = header[0] + 1;

        return 0;
    }
    return -1;
}

int Comm::handlePacket(uint8_t* data, int size, int type)
{
    switch (type)
    {
        case REPORT:
            /*
                Store received data
            */
            std::memcpy(lastPacket, data, size);
            updated = true;
            break;

        case STRUCT_CONF:
            /*
                Update the structure
            */
            structure.clear();
            int j = 0;

            for (int i = 0; i < size; i++) {
                    structure.push_back(std::string((char*) (data + i)));
                    i += structure[j++].length();
            }

            synced = true;
            break;

    };

    return 0;
}
void Comm::receiverCallback(uint8_t* data, int size)
{
    processRawData(data, size);
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
        if (field == "") break;

        // copy stringdata, and update index
        std::strcpy(data + i, field.c_str());
        i += field.length() + 1;
    }

    // send
    sendData((uint8_t*) data, dataSize, STRUCT_CONF);
    std::free(data);

    return 0;
}

bool Comm::getSynced()
{
    return synced;
}

bool Comm::isUpdated()
{
    if (!updated) return false;
    updated = false;
    return true;
}

Comm::Comm(int (*writeHAL)(uint8_t*, int)) : writeHAL(writeHAL) {}

Comm::~Comm()
{
    std::free(inBuff);
    std::free(outBuff);
}

/*Comm* cp2 = nullptr;
Comm* cp1 = nullptr;

int write1(uint8_t* d, int s) {
    cp2->receiverCallback(d, s);
    return 0;
}

int write2(uint8_t* d, int s) {
    cp1->receiverCallback(d, s);
    return 0;
}

int main() 
{   
    
    Comm comm1(write1);
    cp1 = &comm1;

    Comm comm2(write2);
    cp2 = &comm2;

    comm1.addField<int>("itest");
    comm1.addField<std::string>("stest", 4);
    comm1.addField<double>("dtest");

    comm1.sendStructure();

    comm1.setField("itest", 420);
    comm1.setField<std::string>("stest", "cock");
    comm1.setField("dtest", 420.69);

    comm1.sendReport();

    std::cout << comm2.getField<int>("itest") << "\n";
    std::cout << comm2.getField<std::string>("stest") << "\n";
    std::cout << comm2.getField<double>("dtest") << "\n";

    return 0;
}*/