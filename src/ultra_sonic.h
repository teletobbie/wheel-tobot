/**
 * @file ultra_sonic.h
 * @brief Ultrasonic Sensor Interface
 *
 * Provides high-level control for an ultrasonic sensor.
 * Supports distance measurement with trigger and echo pins.
 * Placed on front of the robot for obstacle detection and to make sure the robot doesn't run into things.
 */

#ifndef ULTRA_SONIC_H
#define ULTRA_SONIC_H

void SetupUltraSonic();
float MeasureDistance();
void TestUltraSonicSensor();

#endif