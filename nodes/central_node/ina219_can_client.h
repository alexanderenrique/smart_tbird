/*
 * INA219 CAN Client Header
 * =======================
 * 
 * This header provides function declarations for INA219 CAN communication.
 */

#ifndef INA219_CAN_CLIENT_H
#define INA219_CAN_CLIENT_H

#include "../shared/can_lib/can_messages.h"

// Function declarations
bool sendINA219BatteryData(const INA219Data& battery_data);
void handleINA219CANMessages();
void printINA219CANStatus();

#endif // INA219_CAN_CLIENT_H
