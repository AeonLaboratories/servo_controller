///////////////////////////////////////////////////////
// error.h
//
// servo controller

#pragma once // Include this file only once

#include "..\\..\\common_controller\\include\\c99types.h"

#define ERROR_NONE			0		// No error
#define ERROR_ADC			1		// adc out of range
#define ERROR_BUF_OVFL		2		// RSS232 input buffer overflow
#define ERROR_CRC			4		// RS232 CRC error
#define ERROR_COMMAND		8		// unrecognized command from RS232
#define ERROR_T1RANOVER		16		// Timer 1 still running when Timer 0 reset
#define ERROR_CHANNEL		32		// Invalid device channel
#define ERROR_DATALOG		64		// Datalogging interval out of range (0..255)
#define ERROR_SERVO			128		// Servo out of range
#define ERROR_CPW 			256		// Servo control pulse width out of range
#define ERROR_TIMEOUT		512		// StopOnT out of range
#define ERROR_BOTH_LIMITS	1024	// both limit switches activated?
#define ERROR_LOW_POWER		2048	// low Servo Power Supply Voltage
#define ERROR_ILIM			4096	// StopOnI out of range
#define ERROR_LIMSW			8192	// unrecognized stop limit


extern volatile uint16_t Error;
