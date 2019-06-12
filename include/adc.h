///////////////////////////////////////////////////////
// adc.h
//
// Servo Controller
// Analog Inputs configuration

#ifndef ADC_H
#define ADC_H

#include "..\\..\\common controller\\include\\mask.h"

// ANA6 = disable (pin is configured PC2:OUT, signal is ADDR5)
// ANA5 = disable (pin is configured PC1:IN, signal is -LIMIT0)
// ANA4 = disable (pin is configured PC0:IN, signal is -LIMIT1)
// ANA3 = no connect (pin is configured PB3:OUT)
// ANA2 = SERVO_V
// ANA1 = SERVO_I
// ANA0 = disable (pin is convigured for PB0:OUT, signal is ADDR3)

#define SERVO_I_SENSE		0x01
#define SERVO_V_SENSE		0x02

// ADCCTL0 - ADC Control Register 0
// bit 7 CEN 		= 0		// ADC Conversion Enable
// bit 6 REFSELL	= 0		// use 2.0V internal reference (default is 0 on reset)
// bit 5 REFEXT 	= 0		// provide buffered reference voltage Vref pin?
// bit 4 CONT		= 0		// single-shot mode
// bit 3 ANAIN3		= 0		// Analog Input select ANAIN[3:0]
// bit 2 ANAIN2		= 0
// bit 1 ANAIN1		= 0
// bit 0 ANAIN0		= 0

#define ADC_CHANNEL 0x0F	// ANAIN[3:0]

#define adc_select(x)	mask_clr(ADCCTL0, ADC_CHANNEL), mask_set(ADCCTL0, x)

// Setting ADCCTL0 CEN (bit 7) intitiates a single-shot conversion.
#define ADC_CEN			0x80	// ADCCTL0 CEN (bit 7)
#define adc_start()		mask_set(ADCCTL0, ADC_CEN)
#define adc_disable()	mask_clr(ADCCTL0, ADC_CEN)

// ADCCTL0 CEN (bit 7) is cleared by eZ8 when conversion is complete.
#define adc_busy()		(ADCCTL0 & ADC_CEN)

// ADCCTL1 - ADC Control Register 1
// bit 7 REFSELH	= 1		// 2.0V internal reference (default is 1 on reset)
// bit 6 reserved	= 0
// bit 5 reserved	= 0
// bit 4 reserved	= 0
// bit 3 reserved	= 0
// bit 2 reserved	= 0
// bit 1 reserved	= 0
// bit 0 reserved	= 0



///////////////////////////////////////////////////////
// prototypes
//
void init_adc(void);
unsigned int adc_read( void );

#endif