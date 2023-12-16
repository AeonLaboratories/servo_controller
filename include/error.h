///////////////////////////////////////////////////////
// error.h
//
// servo controller

#pragma once // Include this file only once

#include "..\\..\\common_controller\\include\\c99types.h"

#define ERROR_NONE			0		// no error
#define ERROR_ADC			1		// adc out of range
#define ERROR_BUF_OVFL		2		// RSS232 input buffer overflow
#define ERROR_CRC			4		// RS232 CRC error
#define ERROR_COMMAND		8		// unrecognized command from RS232
#define ERROR_CHANNEL		16		// invalid device channel
#define ERROR_DATALOG		32		// datalogging interval out of range (0..255)

#define ERROR_CPW 			64		// control pulse width out of range
#define ERROR_ILIM			128		// StopOnI out of range
#define ERROR_TIMEOUT		256		// StopOnT out of range
#define ERROR_LIMSW			512		// unrecognized stop limit

#define ERROR_BOTH_LIMITS	1024	// both limit switches activated?
#define ERROR_LOW_POWER		2048	// low Servo Power Supply Voltage


extern volatile uint16_t Error;
