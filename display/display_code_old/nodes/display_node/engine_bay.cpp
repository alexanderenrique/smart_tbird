#include <Arduino.h>
#include <temp_table.h>
#include "ModbusServerRTU.h"
#include "RTUutils.h"
#include <driver/gpio.h>

// Modbus Configuration
#define RX_PIN 20  // RS-485 RX pin (adjust to your wiring)
#define TX_PIN 21  // RS-485 TX pin (adjust to your wiring)
#define DE_RE_PIN 0     // GPIO, can change this later
#define COOLANT_ADC_PIN 1 // GPIO, can change this later
#define OIL_ADC_PIN 2 // GPIO, can change this later
#define TRANS_ADC_PIN 3 // GPIO, can change this later

#define SLAVE_ID 1

#define MODBUS_BAUD_RATE 9600 
#define COOLANT_TEMP_ADDRESS 0
#define OIL_TEMP_ADDRESS 1
#define TRANS_TEMP_ADDRESS 2

HardwareSerial RS485(1);
ModbusServerRTU MBserver(2000, DE_RE_PIN); // timeout in ms, RTS pin

// Modbus holding registers array - this is what the master reads from
// Index 0 = Coolant temp, Index 1 = Oil temp, Index 2 = Trans temp
static uint16_t holding_regs[3] = {0, 0, 0};

static int16_t coolant_temp = 0;
static int16_t oil_temp = 0;
static int16_t trans_temp = 0;

// Modbus worker function to handle READ_HOLD_REGISTER requests
ModbusMessage handleReadHoldingRegisters(ModbusMessage request) {
    uint16_t startAddress;
    uint16_t numRegisters;
    ModbusMessage response;

    // Extract start address and number of registers from request
    request.get(2, startAddress);
    request.get(4, numRegisters);

    // Validate address range
    if (startAddress + numRegisters > 3) {
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
        return response;
    }

    // Build response with register values
    response.add(request.getServerID(), request.getFunctionCode(), numRegisters * 2);
    for (uint16_t i = 0; i < numRegisters; ++i) {
        response.add(holding_regs[startAddress + i]);
    }

    return response;
}

void setup() {
    Serial.begin(9600);
    
    // Prepare hardware serial for Modbus RTU
    RTUutils::prepareHardwareSerial(RS485);
    
    // Initialize RS485 serial port
    RS485.begin(MODBUS_BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
    
    // Configure RS485 direction pin
    pinMode(DE_RE_PIN, OUTPUT);
    digitalWrite(DE_RE_PIN, LOW); // start in receive mode

    // Configure GPIO pins for ADC
    pinMode(COOLANT_ADC_PIN, INPUT);
    pinMode(OIL_ADC_PIN, INPUT);
    pinMode(TRANS_ADC_PIN, INPUT);
    gpio_set_pull_mode((gpio_num_t)COOLANT_ADC_PIN, GPIO_FLOATING);
    gpio_set_pull_mode((gpio_num_t)OIL_ADC_PIN, GPIO_FLOATING);
    gpio_set_pull_mode((gpio_num_t)TRANS_ADC_PIN, GPIO_FLOATING);
    
    // Initialize Modbus RTU server
    MBserver.begin(RS485, MODBUS_BAUD_RATE, SERIAL_8N1, 2000); // Serial, baud rate, config, timeout
    
    // Register worker function to handle READ_HOLD_REGISTER (function code 0x03) requests
    MBserver.registerWorker(SLAVE_ID, READ_HOLD_REGISTER, &handleReadHoldingRegisters);
    
    // Configure ADC pins for ESP32-C3
    // ESP32-C3 uses different ADC API - set global attenuation
    analogSetAttenuation(ADC_11db);  // 11dB attenuation allows 0-3.3V input range
}

// Pass the pin into this function
// then we'll read the adc value and pass it to the Calc_Temp_fromADC function
// then we'll do our smoothing and then we'll pass the value to the global variables    
void takeMeasurements(int16_t pin_number) {
    // Take 10 readings and average them
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(pin_number);
        delay(10);
    }
    int adc_value = sum / 10;

    // Convert ADC value to temperature
    float read_temp = Calc_Temp_fromADC(adc_value);
    // TODO: Add smoothing/filtering here if needed

    // Store the temperature in the appropriate global variable and Modbus register
    // Temperature is stored as tenths of a degree (e.g., 185 = 18.5°F) for extra precision
    int16_t temp_scaled = (int16_t)(read_temp * 10);
    
    if (pin_number == COOLANT_ADC_PIN) {
        coolant_temp = temp_scaled;
        holding_regs[COOLANT_TEMP_ADDRESS] = (uint16_t)temp_scaled;  // Write to Modbus register 0
    } else if (pin_number == OIL_ADC_PIN) {
        oil_temp = temp_scaled;
        holding_regs[OIL_TEMP_ADDRESS] = (uint16_t)temp_scaled;      // Write to Modbus register 1
    } else if (pin_number == TRANS_ADC_PIN) {
        trans_temp = temp_scaled;
        holding_regs[TRANS_TEMP_ADDRESS] = (uint16_t)temp_scaled;    // Write to Modbus register 2
    }
}

void loop() {
    // Take measurements from all temperature sensors
    takeMeasurements(COOLANT_ADC_PIN);
    delay(100);
    takeMeasurements(OIL_ADC_PIN);
    delay(100);
    takeMeasurements(TRANS_ADC_PIN);
    delay(100);
    // Note: eModbus server runs automatically in the background, no need for mb.task()
}