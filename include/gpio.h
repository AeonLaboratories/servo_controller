///////////////////////////////////////////////////////
// gpio.h
//
// servo controller

#pragma once // Include this file only once

#include <ez8.h>
#include "..\\..\\common_controller\\include\\mask.h"

///////////////////////////////////////////////////////
//
// Conventions for configuring gpio pins
// 	(Note: keep DBG & -RESET reserved for flashing/debugging)
//
// UART uses PA4/RX, PA5/TX
//
// Preferred analog inputs:
//		PB0/ANA0 .. PB3/ANA3
//
// Preferred digital inputs
// 		PC0 .. PC3
//
// Preferred digital outputs (open-drain or push-pull possible)
// 		PA0, PA1, PA2, PA3, PA6, PA7
//
// Alternate/additional analog inputs:
//		PC0/ANA4 .. PC2/ANA6
//
// Alternate/additional digital outputs (push-pull only)
//		PC0/ANA4 .. PC2/ANA6
//		PB0/ANA0 .. PB3/ANA3
//
///////////////////////////////////////////////////////

///////////////////////////////////////////////////////
//
// NOTE: Open drain mode is only available on Port A gpio pins
// (See "Errata for Z8 Encore XP F082A Series UP0069.pdf")
//
///////////////////////////////////////////////////////

///////////////////////////////////////////////////////
// Port A
// PA7 = OUT: -SERVO_CP
// PA6 = IN:  no connect
// PA5 = OUT: TXD0 (Alt. function)
// PA4 = IN:  RXD0 (Alt. function)
// PA3 = OUT: ADDR2
// PA2 = OUT: ADDR1
// PA1 = OUT: ADDR0
// PA0 = IN:  no connect
//
// DD == data direction
// OC == output control
// AF == alternate function
//
// PADD		= 00010000		// 1 = IN; 0 = OUT (set unused pins to OUT)
// PAOC		= 00001110		// 1 = open drain; 0 = push-pull (the default)
// PAAF		= 00110000		// alternate functions
// PAOUT	= 10000000		// defaults
#define PA_DD					0x10
#define PA_OC					0x0E
#define PA_AF					0x30
#define PA_OUT					0x80

// this is for -SERVO_CP so logic is reversed
#define SERVO_CP				0x80
#define SERVO_CP_low()			mask_set(PAOUT, SERVO_CP)
#define SERVO_CP_high()			mask_clr(PAOUT, SERVO_CP)
#define SERVO_CP_is_low()		((SERVO_CP & PAOUT) == SERVO_CP)
#define SERVO_CP_is_high()		((SERVO_CP & PAOUT) == 0)


///////////////////////////////////////////////////////
// Port B
// PB7 = N/A
// PB6 = N/A
// PB5 = N/A
// PB4 = N/A
// PB3 = IN:  no connect
// PB2 = IN:  ANA2 (Alt. function) = SERVO_V
// PB1 = IN:  ANA1 (Alt. function) = SERVO_I
// PB0 = OUT: ADDR3
//
// PBDD		= 00000110		// 1 = IN; 0 = OUT (set unused pins to OUT)
// PBAF		= 00000110		// alternate functions
// PBOUT	= 00000000		// defaults
//
// NOTE: open drain mode does not work for Port B gpio pins
// (See "Errata for Z8 Encore XP F082A Series UP0069.pdf")
//
#define PB_DD					0x06
#define PB_AF					0x06
#define PB_OUT					0x00


///////////////////////////////////////////////////////
// Port C
// PC7 = N/A
// PC6 = N/A
// PC5 = N/A
// PC4 = N/A
// PC3 = OUT: ADDR4
// PC2 = OUT: ADDR5
// PC1 = IN: -LIMIT0
// PC0 = IN: -LIMIT1
//
// PCDD		= 00000011		// 1 = IN; 0 = OUT (set unused pins to OUT)
// PCAF		= 00000000		// alternate functions
// PCOUT	= 00000000		// default values
//
// NOTE: open drain mode does not work for Port C gpio pins
// (See "Errata for Z8 Encore XP F082A Series UP0069.pdf")
//
#define PC_DD					0x03
#define PC_AF					0x00
#define PC_OUT					0x00

#define LIMIT0					0x02
#define LIMIT0_detected()		!(PCIN & LIMIT0)

#define LIMIT1					0x01
#define LIMIT1_detected()		!(PCIN & LIMIT1)


///////////////////////////////////////////////////////
// prototypes
//
void init_gpio(void);
