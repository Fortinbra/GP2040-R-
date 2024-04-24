#include <stdio.h>
#include <hardware/i2c.h>
#include <pico/i2c_slave.h>
#include <pico/stdlib.h>
#include <string.h>
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "headers/GamepadState.h"
#include "headers/I2CData.h"
#include "headers/NESController.h"
#include "headers/SNESController.h"
#include "headers/N64.h"
#include <pico/multicore.h>
#include "hardware/uart.h"
#include <cstdlib>

#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define I2C_SLAVE_ADDR 0xCE
#define I2C_BAUDRATE 100000
#define I2C_SLAVE_SDA_PIN 12
#define I2C_SLAVE_SCL_PIN 13
#ifndef GPCOMMS_BUFFER_SIZE
#define GPCOMMS_BUFFER_SIZE 100
#endif
static GamepadState gamepadState = {};
Mask_t gpioState = 0;
NESController nesController(2, 3, 4);
SNESController snesController(2, 3, 4);
N64 n64Controller(4);

void processI2CData()
{
    // Translate the gamepad data into NES format
    uint16_t nesData = nesController.translateToFormat(gamepadState);

    // Send the data to the NES system
    nesController.sendToSystem(nesData);
}

void handleStatus(uint8_t *payload)
{
    (void)0;
}

void handleMessage(uint8_t *payload)
{
    (void)0;
}

static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event)
{
    static uint8_t buf[GPCOMMS_BUFFER_SIZE] = {0};
    static uint8_t receivedIndex = 0;

    switch (event)
    {
    case I2C_SLAVE_RECEIVE:
        buf[receivedIndex] = i2c_read_byte_raw(i2c);
        receivedIndex++;
        break;
    case I2C_SLAVE_REQUEST:
        // TODO: Do something useful?!?
        break;
    case I2C_SLAVE_FINISH:
    {
        uint8_t command = buf[0];
        uint8_t *payload = &buf[1];

        switch (command)
        {
        case GPCMD_STATE:
            static GPComms_State gpState = {};
            gamepadState.buttons = 0;
            gamepadState.dpad = 0;
            memcpy(&gpState, payload, sizeof(GPComms_State));
            gamepadState = gpState.gamepadState;
            gpioState = gpState.gpioState;
            break;

        // case GPCMD_STATUS:
        //     handleStatus(payload);
        //     break;

        // case GPCMD_MESSAGE:
        //     handleMessage(payload);
        //     break;

        // case GPCMD_ACK:
        //     break;

        // case GPCMD_UNKNOWN:
        default:
            break;
        }
        receivedIndex = 0;
        break;
    }
    default:
        break;
    }
}
void setup_slave()
{
    gpio_init(I2C_SLAVE_SDA_PIN);
    gpio_set_function(I2C_SLAVE_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SLAVE_SDA_PIN);

    gpio_init(I2C_SLAVE_SCL_PIN);
    gpio_set_function(I2C_SLAVE_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SLAVE_SCL_PIN);

    i2c_init(i2c0, I2C_BAUDRATE);
    i2c_slave_init(i2c0, I2C_SLAVE_ADDR, &i2c_slave_handler);
}
void core1()
{
    multicore_lockout_victim_init(); // block core 1
    setup_slave();
    while (true)
    {
    };
}
int main()
{
    stdio_init_all();
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    multicore_launch_core1(core1);
    while (true)
    {
        processI2CData();
        sleep_ms(1000);
    };
}