#include <comm.hpp>
#include <string>
#include <stdio.h>
#include <LoRa-RP2040.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include <cmath>

#define PIN_SDA 0
#define PIN_SCL 1

#define ADDR_MPU 0x69
#define ADDR_BMP 0x76

#define BMP_CMD 0x7E
#define BMP_PWR_CTL 0x1B

#define BMP_RST 0xB6
#define BMP_OSR 0x1C
#define BMP_CONF 0x1F
#define BMP3_REG_CALIB_DATA 0x31
#define BMP3_REG_DATA 0x04

#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define GYRO_XOUT_H 0x43
#define ACCEL_XOUT_H 0x3B

#define FS_SEL 1
#define AFS_SEL 1

struct calib_data {
    double par_t1;
    double par_t2;
    double par_t3;
    double par_p1;
    double par_p2;
    double par_p3;
    double par_p4;
    double par_p5;
    double par_p6;
    double par_p7;
    double par_p8;
    double par_p9;
    double par_p10;
    double par_p11;
    double t_lin;
};

struct bmp3_data {
  double temperature;
  double pressure;
  bool success;
};

calib_data _calib_data;

void readReg(uint8_t addr, uint8_t reg, uint8_t* buff, uint8_t len)
{
    i2c_write_blocking(i2c0, addr, &reg, 1, true);
    i2c_read_blocking(i2c0, addr, buff, len, false);
}

void parse_calib_data(const uint8_t* reg_data) {
    auto u16 = [](uint8_t msb, uint8_t lsb) { return (uint16_t)(((uint16_t)msb << 8) | lsb); };

    uint16_t par_t1_u = u16(reg_data[1], reg_data[0]);
    uint16_t par_t2_u = u16(reg_data[3], reg_data[2]);
    int8_t   par_t3_s = (int8_t)reg_data[4];

    int16_t  par_p1_s = (int16_t)u16(reg_data[6], reg_data[5]);
    int16_t  par_p2_s = (int16_t)u16(reg_data[8], reg_data[7]);
    int8_t   par_p3_s = (int8_t)reg_data[9];
    int8_t   par_p4_s = (int8_t)reg_data[10];
    uint16_t par_p5_u = u16(reg_data[12], reg_data[11]);
    uint16_t par_p6_u = u16(reg_data[14], reg_data[13]);
    int8_t   par_p7_s = (int8_t)reg_data[15];
    int8_t   par_p8_s = (int8_t)reg_data[16];
    int16_t  par_p9_s = (int16_t)u16(reg_data[18], reg_data[17]);
    int8_t   par_p10_s = (int8_t)reg_data[19];
    int8_t   par_p11_s = (int8_t)reg_data[20];

    _calib_data.par_t1 = (double)par_t1_u / 0.00390625f;
    _calib_data.par_t2 = (double)par_t2_u / 1073741824.0f;
    _calib_data.par_t3 = (double)par_t3_s / 281474976710656.0f;

    _calib_data.par_p1 = ((double)par_p1_s - 16384.0) / 1048576.0f;
    _calib_data.par_p2 = ((double)par_p2_s - 16384.0) / 536870912.0f;
    _calib_data.par_p3 = (double)par_p3_s / 4294967296.0f;
    _calib_data.par_p4 = (double)par_p4_s / 137438953472.0f;
    _calib_data.par_p5 = (double)par_p5_u / 0.125f;
    _calib_data.par_p6 = (double)par_p6_u / 64.0f;
    _calib_data.par_p7 = (double)par_p7_s / 256.0f;
    _calib_data.par_p8 = (double)par_p8_s / 32768.0f;
    _calib_data.par_p9 = (double)par_p9_s / 281474976710656.0f;
    _calib_data.par_p10 = (double)par_p10_s / 281474976710656.0f;
    _calib_data.par_p11 = (double)par_p11_s / 36893488147419103232.0f;
}
bool get_calib_data() {
    uint8_t calib_buffer[21];
    readReg(ADDR_BMP, BMP3_REG_CALIB_DATA, calib_buffer, 21);
    
    bool all_zeros = true;
    for (int i = 0; i < 21; ++i) {
        if (calib_buffer[i] != 0) {
            all_zeros = false;
            break;
        }
    }
    if (all_zeros) {
        printf("Calib data zero.\n");
        return false;
    }
    
    parse_calib_data(calib_buffer);
    return true;
}

// you should supply a function that can send a packet to the receiver
// the max possible packet size is 255 bytes
// the arguments should be the buffer and the size of it
int send(uint8_t* data, int size) {
    LoRa.beginPacket();
    LoRa.write(data, size);
    LoRa.endPacket();
    return 0;
}

int16_t read_i16(uint8_t reg)
{
    uint8_t buff[2];

    i2c_write_blocking(i2c0, ADDR_MPU, &reg, 1, true);
    i2c_read_blocking(i2c0, ADDR_MPU, buff, 2, false);
    int16_t val = (buff[1] << 8 )| buff[0];

    if (val & 0x8000) val -= 0x10000;

    return val;
}

uint8_t readByte(uint8_t addr, uint8_t reg)
{
    uint8_t ret;
    i2c_write_blocking(i2c0, addr, &reg, 1, true);
    i2c_read_blocking(i2c0, addr, &ret, 1, false);

    return ret;
}


int read_accel(float *x, float *y, float *z)
{
    *x = (float) read_i16(ACCEL_XOUT_H) / (16384.0 / pow(2, AFS_SEL));
    *y = (float) read_i16(ACCEL_XOUT_H + 2) / (16384.0 / pow(2, AFS_SEL));
    *z = (float) read_i16(ACCEL_XOUT_H + 4) / (16384.0 / pow(2, AFS_SEL));

    return 0;
}

void writeReg(uint8_t addr, uint8_t reg, uint8_t val)
{
    uint8_t buff[] = {reg, val};
    i2c_write_blocking(i2c0, addr, buff, 1, false);
}

double compensate_temperature(uint32_t uncomp_temp) {
    double partial_data1 = (double)(uncomp_temp - _calib_data.par_t1);
    double partial_data2 = (double)(partial_data1 * _calib_data.par_t2);
    _calib_data.t_lin = partial_data2 + (partial_data1 * partial_data1) * _calib_data.par_t3;
    return _calib_data.t_lin;
}

double compensate_pressure(uint32_t uncomp_press) {
    double partial_data1 = _calib_data.par_p6 * _calib_data.t_lin;
    double partial_data2 = _calib_data.par_p7 * (_calib_data.t_lin * _calib_data.t_lin);
    double partial_data3 = _calib_data.par_p8 * (_calib_data.t_lin * _calib_data.t_lin * _calib_data.t_lin);
    double partial_out1 = _calib_data.par_p5 + partial_data1 + partial_data2 + partial_data3;

    partial_data1 = _calib_data.par_p2 * _calib_data.t_lin;
    partial_data2 = _calib_data.par_p3 * (_calib_data.t_lin * _calib_data.t_lin);
    partial_data3 = _calib_data.par_p4 * (_calib_data.t_lin * _calib_data.t_lin * _calib_data.t_lin);
    double partial_out2 = (double)uncomp_press * (_calib_data.par_p1 + partial_data1 + partial_data2 + partial_data3);
    
    partial_data1 = (double)uncomp_press * (double)uncomp_press;
    double partial_data_p9_p10 = _calib_data.par_p9 + _calib_data.par_p10 * _calib_data.t_lin;
    double partial_data3_calc = partial_data1 * partial_data_p9_p10;
    double partial_data4 = partial_data3_calc + ((double)uncomp_press * (double)uncomp_press * (double)uncomp_press) * _calib_data.par_p11;

    return partial_out1 + partial_out2 + partial_data4;
}

bmp3_data get_bmp_values() {
    bmp3_data sensor_data;
    sensor_data.success = false;

    uint8_t reg_data[6];
    readReg(ADDR_BMP, BMP3_REG_DATA, reg_data, 6);

    uint32_t uncomp_press = (uint32_t)((reg_data[2] << 16) | (reg_data[1] << 8) | reg_data[0]);
    uint32_t uncomp_temp = (uint32_t)((reg_data[5] << 16) | (reg_data[4] << 8) | reg_data[3]);
    
    if (uncomp_press == 0 && uncomp_temp == 0) {
        printf("Raw data zero.\n");
        return sensor_data;
    }
    
    sensor_data.temperature = compensate_temperature(uncomp_temp);
    sensor_data.pressure = compensate_pressure(uncomp_press);
    
    sensor_data.success = true;
    return sensor_data;
}


int main() {
    stdio_init_all();

    sleep_ms(2000);
    printf("start pls...");

    i2c_init(i2c0, 400000);

    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);

    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PIN_SDA, PIN_SCL, GPIO_FUNC_I2C));

    printf("read id: %u\n", readByte(ADDR_BMP, 0x00));

    printf("soft resetting BMP390...\n");
    writeReg(ADDR_BMP, BMP_CMD, BMP_RST);

    if (!get_calib_data())
    {
        printf("failed to get calibration data\n");
        return 1;
    } 

    printf("enabling readings...\n");
    writeReg(ADDR_BMP, BMP_CMD, 0b001 << 4 | 0b11);

    writeReg(ADDR_BMP, BMP_OSR, 0x0A);
    writeReg(ADDR_BMP, BMP_CONF, 0x03);


    for (;;)
    {
        bmp3_data data = get_bmp_values();

        if (data.success) printf("P: %d T: %d\n", data.pressure, data.temperature);
        sleep_ms(500);
    }

    uint8_t buff[] = {ACCEL_CONFIG, (AFS_SEL << 3)};
    i2c_write_blocking(i2c0, ADDR_MPU, buff, 2, false);

    for(;;)
    {
        float x, y, z;

        read_accel(&x, &y, &z);

        printf("accel data: x:%f y:%f z:%f\n", x, y, z);

        sleep_ms(200);
    }

    sleep_ms(2000);
    printf("starting...\n");

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