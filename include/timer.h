///////////////////////////////////////////////////////
// timer.h
//
// Servo Controller
// Timer Configuration

#ifndef TIMER_H
#define TIMER_H

#include "..\\..\\common controller\\include\\mask.h"

#define SYS_FREQ		5529600uL	// is there a way to get this number?



///////////////////////////////////////////////////////
// Timer control register 1 bits
#define TIMER_ENABLE			0x80
#define TIMER_INVERT			0x40
// The timer prescale divides the system clock
// to create the timer's counter clock. The prescale
// is a power of two from 1 to 128. With the system
// clock driven by the internal precision oscillator (IPO)
// at 5,529,600 Hz, the timer clock's frequency
// ranges from 43,200 Hz (prescale = 128)
// to 5,529,600 Hz (prescale = 1).
// With the system clock driven by a crystal at 
// 18,432,000 Hz, the timer clock's frequency
// ranges from 144,000 Hz to 18,432,000 Hz.
//
// The prescale value is set in TxCTL1[3:5]
//    00xx x000
#define TIMER_MODE_ONESHOT		0x00
#define TIMER_MODE_CONTINUOUS	0x01
#define TIMER_MODE_COUNTER		0x02
#define TIMER_MODE_PWM			0x03
#define TIMER_MODE_CAPTURE		0x04
#define TIMER_MODE_COMPARE		0x05
#define TIMER_MODE_GATED		0x06
#define TIMER_MODE_CAPCOMP		0x07



///////////////////////////////////////////////////////
// Timer 0
// Generates an interrupt every 20 ms to initiate a
// servo control pulse.
//

#define T0_PRESCALE		128

#if (T0_PRESCALE == 128)
#define T0_PRESCALE_BITS	0x38
#elif (T0_PRESCALE == 64)
#define T0_PRESCALE_BITS	0x30
#elif (T0_PRESCALE == 32)
#define T0_PRESCALE_BITS	0x28
#elif (T0_PRESCALE == 16)
#define T0_PRESCALE_BITS	0x20
#elif (T0_PRESCALE == 8)
#define T0_PRESCALE_BITS	0x18
#elif (T0_PRESCALE == 4)
#define T0_PRESCALE_BITS 	0x10
#elif (T0_PRESCALE == 2)
#define T0_PRESCALE_BITS	0x08
#elif (T0_PRESCALE == 1)
#define T0_PRESCALE_BITS	0x00
#endif

// Control register 1:
// If TIMER_INVERT is included, the timer Polarity
// bit is set.
#define T0_CONFIG		(TIMER_MODE_CONTINUOUS | T0_PRESCALE_BITS)

// use FREQ instead of PERIOD for frequencies above 1 Hz
// use PERIOD instead of FREQ for frequencies below 1 Hz
#define T0_FREQ			50.0
//#define T0_PERIOD		1.0

#ifndef T0_FREQ
#define T0_FREQ			(1.0/T0_PERIOD)
#define T0_RESET		SYS_FREQ / T0_PRESCALE * T0_PERIOD
#endif

#ifndef T0_PERIOD
#define T0_PERIOD		(1.0/T0_FREQ)
#define T0_RESET		(SYS_FREQ / T0_PRESCALE / T0_FREQ)
#endif


///////////////////////////////////////////////////////
// Timer 1
// Generates an interrupt to terminate the servo 
// control pulse after the desired pulse width.
//
#define T1_PRESCALE		2

#if (T1_PRESCALE == 128)
#define T1_PRESCALE_BITS	0x38
#elif (T1_PRESCALE == 64)
#define T1_PRESCALE_BITS	0x30
#elif (T1_PRESCALE == 32)
#define T1_PRESCALE_BITS	0x28
#elif (T1_PRESCALE == 16)
#define T1_PRESCALE_BITS	0x20
#elif (T1_PRESCALE == 8)
#define T1_PRESCALE_BITS	0x18
#elif (T1_PRESCALE == 4)
#define T1_PRESCALE_BITS 	0x10
#elif (T1_PRESCALE == 2)
#define T1_PRESCALE_BITS	0x08
#elif (T1_PRESCALE == 1)
#define T1_PRESCALE_BITS	0x00
#endif

// Control register 1:
// If TIMER_INVERT is included, the timer Polarity
// bit is set.
#define T1_CONFIG		(TIMER_MODE_ONESHOT | T1_PRESCALE_BITS)

// use PERIOD instead of FREQ for frequencies below 1 Hz
// use FREQ instead of PERIOD for frequencies above 1 Hz
// One-Shot mode frequency is system frequency / prescale
#define T1_FREQ			SYS_FREQ / T1_PRESCALE
//#define T1_PERIOD		1.0

#ifndef T1_FREQ
#define T1_FREQ			(1.0/T1_PERIOD)
#define T1_RESET		(SYS_FREQ / T1_PRESCALE * T1_PERIOD)
#endif

#ifndef T1_PERIOD
#define T1_PERIOD		(1.0/T1_FREQ)
#define T1_RESET		(SYS_FREQ / T1_PRESCALE / T1_FREQ)
#endif



///////////////////////////////////////////////////////

#define config_timer0(x)	(T0CTL = (x))
#define start_timer0()		(mask_set(T0CTL, TIMER_ENABLE))
#define stop_timer0()		(mask_clr(T0CTL, TIMER_ENABLE))
#define set_timer0_count(x)	(T0 = (x))
#define clear_timer0()		set_timer0_count(0)
#define set_timer0_end(x)	(T0R = (x))		// AKA "reset"
#define set_timer0_mark(x)	(T0CP = (x))

#define config_timer1(x)	(T1CTL = (x))
#define start_timer1()		(mask_set(T1CTL, TIMER_ENABLE))
#define stop_timer1()		(mask_clr(T1CTL, TIMER_ENABLE))
#define set_timer1_count(x)	(T1 = (x))
#define clear_timer1()		set_timer1_count(0)
#define set_timer1_end(x)	(T1R = (x))		// AKA "reset"
#define set_timer1_mark(x)	(T1CP = (x))



///////////////////////////////////////////////////////
// prototypes
//
void init_timer0(void);
void init_timer1(void);


#endif
