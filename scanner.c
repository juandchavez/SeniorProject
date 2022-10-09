/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Scanner implementation is by Nick Poole at SparkFun
 * Electronics
 */


#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

/// \tag::hello_uart[]

// Scanner Definitions
#define DE2120_COMMAND_ACK 0x06
#define DE2120_COMMAND_NACK 0x15

//These are the commands we can send (Prepend "^_^" and append ".")
#define COMMAND_START_SCAN "SCAN"
#define COMMAND_STOP_SCAN "SLEEP"
#define COMMAND_SET_DEFAULTS "DEFALT"
#define COMMAND_GET_VERSION "DSPYFW"

#define PROPERTY_BUZZER_FREQ "BEPPWM"
//BEPPWM0 - Active Drive
//BEPPWM1 - Passive Low Freq
//BEPPWM2 - Passive Med Freq (default)
//BEPPWM3 - Passive Hi Freq

#define PROPERTY_DECODE_BEEP "BEPSUC"
//BEPSUC1 - ON (default)
//BEPSUC0 - OFF

#define PROPERTY_BOOT_BEEP "BEPPWR"
//BEPPWR1 - ON (default)
//BEPPWR0 - OFF

#define PROPERTY_FLASH_LIGHT "LAMENA"
//LAMENA1 - ON (default)
//LAMENA0 - OFF

#define PROPERTY_AIM_LIGHT "AIMENA"
//AIMENA1 - ON (default)
//AIMENA0 - OFF

#define PROPERTY_READING_AREA "IMGREG"
//IMGREG0 - Full Width (default)
//IMGREG1 - Center 80%
//IMGREG2 - Center 60%
//IMGREG3 - Center 40%
//IMGREG4 - Center 20%

#define PROPERTY_MIRROR_FLIP "MIRLRE"
//MIRLRE1 - ON
//MIRLRE0 - OFF (default)

#define PROPERTY_USB_DATA_FORMAT "UTFEAN"
//UTFEAN0 - GBK (default)
//UTFEAN1 - UTF-8

#define PROPERTY_SERIAL_DATA_FORMAT "232UTF"
//232UTF0 - GBK (default)
//232UTF1 - UTF-8
//232UTF2 - Unicode BIG
//232UTF3 - Unicode little

#define PROPERTY_INVOICE_MODE "SPCINV"
//SPCINV1 - ON
//SPCINV0 - OFF (default)

#define PROPERTY_VIRTUAL_KEYBOARD "KBDVIR"
//KBDVIR1 - ON (default)
//KBDVIR0 - OFF

#define PROPERTY_COMM_MODE "POR"
//PORKBD - USB-KBW Mode
//PORHID - USB-HID Mode
//PORVIC - USB-COM Mode
//POR232 - TTL/RS232

#define PROPERTY_BAUD_RATE "232BAD"
//232BAD2 - 1200 bps
//232BAD3 - 2400 bps
//232BAD4 - 4800 bps
//232BAD5 - 9600 bps
//232BAD6 - 19200 bps
//232BAD7 - 38400 bps
//232BAD8 - 57600 bps
//232BAD9 - 115200 bps (default)

#define PROPERTY_READING_MODE "SCM"
//SCMMAN - Manual (default)
//SCMCNT - Continuous
//SCMMDH - Motion Mode

#define PROPERTY_CONTINUOUS_MODE_INTERVAL "CNTALW"
//CNTALW0 - Output Once
//CNTALW1 - Output Continuous No Interval
//CNTALW2 - Output Continuous 0.5s Interval
//CNTALW3 - Output Continuous 1s Interval

#define PROPERTY_MOTION_SENSITIVITY "MDTTHR"
//MDTTHR15 - Extremely High Sensitivity
//MDTTHR20 - High Sensitivity (default)
//MDTTHR30 - Highish Sensitivity
//MDTTHR50 - Mid Sensitivity
//MDTTHR100 - Low Sensitivity

#define PROPERTY_TRANSFER_CODE_ID "CIDENA"
//CIDENA1 - Transfer Code ID
//CIDENA0 - Do Not Transfer Code ID (default)

#define PROPERTY_KBD_CASE_CONVERSION "KBDCNV"
//KBDCNV0 - No conversion (default)
//KBDCNV1 - ALL CAPS
//KBDCNV2 - all lowercase
//KBDCNV3 - case-to-case

// Barcode Style Enable/Disable
#define PROPERTY_ENABLE_ALL_1D "ODCENA"
#define PROPERTY_DISABLE_ALL_1D "ODCDIS"
#define PROPERTY_ENABLE_ALL_2D "AQRENA"
#define PROPERTY_DISABLE_ALL_2D "AQRDIS"

#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define TRIG_PIN 4
#define REQUEST 5

#define BUF_LEN 40

// Scanner Functions
bool scanner_begin(void);
bool isConnected(void);
bool sendCmd(const char*, const char*, uint32_t);
bool readBarcode(char* , uint8_t);
//bool factoryDefault(void);
bool lightOn(void);
bool lightOff(void);
bool enableAll2D();
bool disableAll2D();
bool enableMotionSense(uint8_t);
bool disableMotionSense(void);

bool scanner_init(void);
 const uint LED_PIN = PICO_DEFAULT_LED_PIN;
void blink(){
        gpio_put(LED_PIN, 1);
        sleep_ms(50);
        gpio_put(LED_PIN, 0);
        sleep_ms(50);
}
int main() {
    stdio_init_all();
    char scanBuf[BUF_LEN];


    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // UART setup
    uart_init(UART_ID, BAUD_RATE);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, false);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);


    
    // Initialize the trigger pin 
    //gpio_init(TRIG_PIN);

    blink();
    blink();
    blink();
    sleep_ms(100);
    if(scanner_init() == false){
        while(1){
        gpio_put(LED_PIN, 1);
        sleep_ms(50);
        gpio_put(LED_PIN, 0);
        sleep_ms(50);
        }
    }

    while(1){
        if(readBarcode(scanBuf, BUF_LEN)){
            printf("Code found!");
        }
        sleep_ms(200);
        
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
    }

    return 0;
}

bool scanner_init(void){
    if(scanner_begin() == false){
        printf("Scanner not available.");
        return false;
    }

    if(lightOn() == false){
        printf("Light could be turned on!");
        return false;
    }

    if(enableMotionSense(50) == false){
        printf("Motion could not be enabled");
        return false;
    }

    printf("Scanner online!");
    return true;
}
/*
    Initialize the scanner with basic settings
    Will return false if the scanner is not detected
*/
bool scanner_begin(void){
    // Just to hold any unwanted chars on the buffer
    char c = ' ';
    // Check the connection
    if(isConnected() == false){
        // No device connected
        return(false);
    }

    // Clear any remaining incoming chars
    // Avoids the first mis read
    while(uart_is_readable(UART_ID)){
        c = uart_getc(UART_ID);
    }

    // Gets rid of warning
    // Shoudlnt affect the program since its local
    c++;
    // Setup is done
    return true;
}

bool isConnected(){
    uart_init(UART_ID, 9600);

    if (sendCmd(COMMAND_GET_VERSION, "", 800)){ //Takes ~430ms to get firmware version response
        return true;
    }

    uart_init(UART_ID, 115200);

    sleep_ms(10);

    sendCmd(PROPERTY_BAUD_RATE, "5", 500); 

    uart_init(UART_ID, 9600);

    if (sendCmd(COMMAND_GET_VERSION, "", 800)){ //Takes ~430ms to get firmware version response
        return true;
    }

  return false;
}

bool sendCmd(const char* cmd, const char* arg, uint32_t maxWaitInms){
    char cmdStr[14] = {'\0'};
    char start[] = "^_^";
    char end[] = ".";
    uint32_t timeout = 0;
    absolute_time_t time;

    strcat(cmdStr, start);
    strcat(cmdStr, cmd);
    strcat(cmdStr, arg);
    strcat(cmdStr, end);

    
    uart_puts(UART_ID, cmdStr);

    //uart_write_blocking(UART_ID,cmdStr, 14);

    timeout = to_ms_since_boot(time) + maxWaitInms;

    /*
        Im stuck in this loop
        
        I when 'debugging' it seems that im skipping over
        the if statement
            - This means that there is no readable data
                on the RX pin on the pico from the scanner
        
    */
    while(to_ms_since_boot(time) < timeout){
        blink();
        if(uart_is_readable(UART_ID)){
            while(uart_is_readable(UART_ID)){
                char incoming = uart_getc(UART_ID);
                if (incoming == DE2120_COMMAND_ACK)
                    return true;
                else if (incoming == DE2120_COMMAND_NACK)
                    return false;
            }
        }
        sleep_ms(1);
    }
    return false;
}

bool readBarcode(char* resultBuf, uint8_t size){
    
    if(!uart_is_readable(UART_ID)){
        return false;
    }

    bool crFound = false;
    int i = 0;

    for(i = 0; i < size; i++){
        if(resultBuf[i] == '\r')
            crFound = true;
    }

    if(crFound)
        resultBuf[0] = '\0';
    
    for(i = strlen(resultBuf); i < size; i++){
        if(uart_is_readable(UART_ID)){
            resultBuf[i] = uart_getc(UART_ID);
            if(resultBuf[i] == '\r'){
                resultBuf[i+1] = '\0';
                return true;
            }
        }else
            return false;
    }
    return false;
}

// Control the white illumination LED
bool lightOn()
{
  return (sendCmd(PROPERTY_FLASH_LIGHT, "1", 3000));
}
bool lightOff()
{
  return (sendCmd(PROPERTY_FLASH_LIGHT, "0", 3000));
}

// Enable and disable motion sensitive read mode
// if enabling, set the sensitivity level
bool enableMotionSense(uint8_t sensitivity)
{
  // reject invalid sensitivity values
  if (sensitivity == 15 || sensitivity == 20 || sensitivity == 30 || sensitivity == 50 || sensitivity == 100)
  {
    char commandString[4] = {'\0'}; //Max is 100\0

    switch (sensitivity)
    {
    case (15):
      strcat(commandString, "15");
      break;
    case (20):
      strcat(commandString, "20");
      break;
    case (30):
      strcat(commandString, "30");
      break;
    case (50):
      strcat(commandString, "50");
      break;
    case (100):
      strcat(commandString, "100");
      break;
    default:
      break;
    }

    sendCmd(PROPERTY_READING_MODE, "MDH", 3000);
    return (sendCmd(PROPERTY_COMM_MODE, commandString, 3000));
  }
  return (false);
}
bool disableMotionSense()
{
  return (sendCmd(PROPERTY_READING_MODE, "MAN", 3000));
}
