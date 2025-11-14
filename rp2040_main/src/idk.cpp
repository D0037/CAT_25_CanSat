#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "bmp3.h"
#include "mpu6500.h"

// I2C pins and configuration
#define I2C_PORT i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define BMP390_ADDR  0x76
#define MPU6500_ADDR 0x68

// UART configuration for NEO-6M
#define UART_BAUD_RATE 9600

void init_i2c() {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
}

void init_uart() {
    uart_init(uart0, UART_BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
}

bool read_bmp390(float* temperature, float* pressure) {
    struct bmp3_dev dev;
    dev.intf = BMP3_I2C_INTF;
    dev.read = [](uint8_t reg_addr, uint8_t* reg_data, uint32_t len, void* intf_ptr) -> int8_t {
        return i2c_write_blocking(I2C_PORT, BMP390_ADDR, &reg_addr, 1, true) == PICO_ERROR_GENERIC ? -1 :
               i2c_read_blocking(I2C_PORT, BMP390_ADDR, reg_data, len, false) == PICO_ERROR_GENERIC ? -1 : 0;
    };
    dev.write = [](uint8_t reg_addr, const uint8_t* reg_data, uint32_t len, void* intf_ptr) -> int8_t {
        uint8_t buf[len + 1];
        buf[0] = reg_addr;
        memcpy(&buf[1], reg_data, len);
        return i2c_write_blocking(I2C_PORT, BMP390_ADDR, buf, len + 1, false) == PICO_ERROR_GENERIC ? -1 : 0;
    };
    dev.delay_ms = [](uint32_t period, void* intf_ptr) { sleep_ms(period); };
    dev.intf_ptr = NULL;

    if (bmp3_init(&dev) != BMP3_OK) return false;

    struct bmp3_data data;
    if (bmp3_get_sensor_data(BMP3_PRESS | BMP3_TEMP, &data, &dev) != BMP3_OK) return false;

    *temperature = data.temperature;
    *pressure = data.pressure;
    return true;
}

bool read_mpu6500(float* accel_x, float* accel_y, float* accel_z, float* gyro_x, float* gyro_y, float* gyro_z) {
    uint8_t buffer[14];
    uint8_t reg = 0x3B;
    if (i2c_write_blocking(I2C_PORT, MPU6500_ADDR, &reg, 1, true) != 1) return false;
    if (i2c_read_blocking(I2C_PORT, MPU6500_ADDR, buffer, 14, false) != 14) return false;

    int16_t ax = (buffer[0] << 😎 | buffer[1];
    int16_t ay = (buffer[2] << 😎 | buffer[3];
    int16_t az = (buffer[4] << 😎 | buffer[5];
    int16_t gx = (buffer[8] << 😎 | buffer[9];
    int16_t gy = (buffer[10] << 😎 | buffer[11];
    int16_t gz = (buffer[12] << 😎 | buffer[13];

    *accel_x = ax / 16384.0f;
    *accel_y = ay / 16384.0f;
    *accel_z = az / 16384.0f;
    *gyro_x = gx / 131.0f;
    *gyro_y = gy / 131.0f;
    *gyro_z = gz / 131.0f;

    return true;
}

bool read_neo6m_line(char* buffer, size_t max_len) {
    size_t i = 0;
    while (i < max_len - 1) {
        if (uart_is_readable(uart0)) {
            char c = uart_getc(uart0);
            if (c == '\n') {
                buffer[i] = '\0';
                return true;
            }
            buffer[i++] = c;
        }
    }
    buffer[max_len - 1] = '\0';
    return false;
}