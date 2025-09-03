/*
 * MPU6050 CAN Client Header
 * ========================
 * 
 * This header provides function declarations for MPU6050 CAN communication.
 */

#ifndef MPU6050_CAN_CLIENT_H
#define MPU6050_CAN_CLIENT_H

#include "../shared/can_lib/can_messages.h"

// Function declarations
bool sendMPU6050Data(const MPU6050Data& data);
bool sendMPU6050MaxValues(const MPU6050MaxData& max_data);
void handleMPU6050CANMessages();
void printMPU6050CANStatus();

#endif // MPU6050_CAN_CLIENT_H
