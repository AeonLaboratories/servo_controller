///////////////////////////////////////////////////////
// gpio.c
//
// Servo Controller

#include <eZ8.h>
#include "gpio.h"

////////////////////////////////////////////////////////
void init_gpio(void)
{
	PAOUT	= PA_OUT;		// default state
	PAOC	= PA_OC;		// open-drain, push-pull config
	PAAF	= PA_AF;		// alternate functions
	PADD	= PA_DD;		// data direction (in/out)

	PBOUT	= PB_OUT;		// default state
//	PBOC	= n/a;			// output control not supported on Port B
	PBAF	= PB_AF;		// alternate functions
	PBDD	= PB_DD;		// data direction (in/out)

	PCOUT	= PC_OUT;		// default state
//	PCOC	= n/a;			// output control not supported on Port C
	PCAF	= PC_AF;		// alternate functions
	PCDD	= PC_DD;		// data direction (in/out)

}

