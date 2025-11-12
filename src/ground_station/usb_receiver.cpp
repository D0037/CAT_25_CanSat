#include <comm.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <stdexcept>

using namespace std;
using namespace boost;

// Function to configure the serial port
void configureSerialPort(asio::serial_port& serial,
                         const string& portname,
                         unsigned int baud_rate)
{
    // Open the specified serial port
    serial.open(portname);
    // Set the baud rate
    serial.set_option(
        asio::serial_port_base::baud_rate(baud_rate));
}

// Function to read data from the serial port
string readFromSerialPort(asio::serial_port& serial)
{
    char buffer[100]; // Buffer to store incoming data
    system::error_code ec;
    // Read data from the serial port
    size_t len
        = asio::read(serial, asio::buffer(buffer), ec);
    if (ec) {
        cerr << "Error reading from serial port: "
             << ec.message() << endl;
        return "";
    }
    // Return the read data as a string
    return string(buffer, len);
}

string readLine(asio::serial_port& serial)
{
    string line = "";
    for (;;)
    {
        char c;
        system::error_code ec;
        // Read data from the serial port
        size_t len
            = asio::read(serial, asio::buffer(&c, 1), ec);
        if (ec) {
            cerr << "Error reading from serial port: "
                << ec.message() << endl;
            return "";
        }
        // Return the read data as a string
        if (c == '\n') return line;
        line.append(string(&c, 1));
    }

}

// Function to write data to the serial port
void writeToSerialPort(asio::serial_port& serial,
                       const string& message)
{
    system::error_code ec;
    // Write data to the serial port
    asio::write(serial, asio::buffer(message), ec);
    if (ec) {
        cerr << "Error writing to serial port: "
             << ec.message() << endl;
    }
}

int main()
{
    asio::io_context io; // Create an IO service
    // Create a serial port object
    asio::serial_port serial(io);


    try {
        // Configure the serial port (replace with your port
        // name)
        configureSerialPort(serial, "/dev/ttyS1", 115200);
    }
    catch (const std::exception& e) {
        cerr << "Error configuring serial port: "
             << e.what() << endl;
        return 1;
    }

    Comm comm(nullptr);
    uint8_t buff[255];

    cout << "Waiting for sync packet...";

    bool s = false;

    for(;;)
    {
        string d = readLine(serial);
        if (d.compare("packet received:") == 0)
        {
            d = d.substr(16);

            for (int i = 0; i < d.length() / 2; i++)
            {   
                
                char c1 = d.at(2*i);
                char c2 = d.at(2*i+1);

                if (c1 <= '9')
                {
                    buff[i] = c1 - '0';
                }
                else
                {
                    buff[i] = c1 - 'A' + 10;
                }

                if (c2 <= '9')
                {
                    buff[i] |= 16 * (c2 - '0');
                }
                else
                {
                    buff[i] |= 16 * (c2 - 'A' + 10);
                }

            }

            comm.receiverCallback(buff, d.length() / 2);
        }
        
        // this is useful, if you have a receiver-transmitter configuration
        // this will check if a sync packet packet has arrived
        if (comm.getSynced())
        {
            if (!s) cout << "\t\t Sync packet arrived!\n";
            s = true;

            /* Write code here */

            // random examples
            cout << "temperature: " << comm.getField<float>("temp") << " C\n";
            cout << "GPS coords: " << comm.getField<std::string>("GPS") << "\n";
            cout << "pressure " << comm.getField<double>("p") << "kPa \n";
        }

    }

    serial.close(); // Close the serial port
    return 0;
}