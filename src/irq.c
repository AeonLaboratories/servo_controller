///////////////////////////////////////////////////////
// irq.c
//
// Aeon Laboratories SC64 64-channel Servo controller

#include <eZ8.h>
#include <defines.h>
#include "..\\..\\common_controller\\include\\c99types.h"
#include "..\\..\\common_controller\\include\\z082a.h"
#include "..\\..\\common_controller\\include\\timer.h"
#include "..\\..\\common_controller\\include\\adc.h"
#include "..\\..\\common_controller\\include\\uart.h"
#include "..\\..\\common_controller\\include\\irq.h"
#include "config.h"
#include "error.h"
#include "gpio.h"

// Store big strings in ROM to conserve RData and EData space.
rom char FIRMWARE[]	= R"Aeon Laboratories SC64 ";
rom char VERSION[]	= R"V.20170515-0000";

#define SERNO					4

// Compensation values (consolidated) for pre-amps & ADC
// To calibrate, set Gain to 1.0 and Offset to 0.0
//

#if SERNO == 0
#define ADC_OFFSET				1		// what ADC reports when the input is 0V.
#define A1_GAIN					2.5065	// SERVO_I
#define A1_OFFSET				8.3333
#define A2_GAIN					2.0639	// SERVO_V
#define A2_OFFSET				0.0
#endif


// If Nat's controllers are updated, these will need
// recalibration.
//

////// NAT LIFTON'S PURIFICATION LINE SERVO CONTROLLER
#if SERNO == 1
#define ADC_OFFSET				1		// what ADC reports when the input is 0V.
#define A1_GAIN					12.167	// SERVO_I
#define A1_OFFSET				12.0
#define A2_GAIN					7.634	// SERVO_V
#define A2_OFFSET				0.0
#endif

////// NAT LIFTON'S RECIRCULATING LINE SERVO CONTROLLER
#if SERNO == 2
#define ADC_OFFSET				1		// what ADC reports when the input is 0V.
#define A1_GAIN					11.692	// SERVO_I
#define A1_OFFSET				12.0
#define A2_GAIN					7.463	// SERVO_V
#define A2_OFFSET				0.0
#endif

////// NAT LIFTON'S FLOW-THROUGH LINE SERVO CONTROLLER
#if SERNO == 3
#define ADC_OFFSET				1		// what ADC reports when the input is 0V.
#define A1_GAIN					12.165	// SERVO_I
#define A1_OFFSET				37.0
#define A2_GAIN					7.803	// SERVO_V
#define A2_OFFSET				0.0
#endif


////// AEON CEGS 1 SERVO CONTROLLER
#if SERNO == 4
#define ADC_OFFSET				1		// what ADC reports when the input is 0V.
#define A1_GAIN					2.5065	// SERVO_I
#define A1_OFFSET				8.3333
#define A2_GAIN					2.0639	// SERVO_V
#define A2_OFFSET				0.0
#endif

////// TULANE CEGS 1 SERVO CONTROLLER
#if SERNO == 5
#define ADC_OFFSET				-23		// what ADC reports when the input is 0V.
#define A1_GAIN					2.9373	// SERVO_I
#define A1_OFFSET				-4.5
#define A2_GAIN					2.0325	// SERVO_V
#define A2_OFFSET				0.0
#endif


#define CHANNELS				64
#define CHANNEL_NONE			(CHANNELS-1)	// last channel 'none' until ADDR_EN added to hardware

#define CURRENT_MAX				5000	// milliamps
#define TIMEOUT_MAX				30000	// in 100ths; 300 seconds = five minutes

#define V_MIN					5000	// millivolts

#define SKIP_INRUSH				50		// 100ths of a second ('elapsed' units)


// The CO_MAX_RESERVE provides time for the "stop pulse"
// interrupt service routine, plus the time at the start
// of the T0 ISR before the interrupts are re-enabled.
// Making CO_MAX_RESERVE long enough prevents contention between
// the start and stop routines, at the cost of limiting the maximum
// duty cycle.
// If the T1 ISR is minimal, and T0 starts the pulse and re-enables
// interrupts as early as possible, CO_MAX_RESERVE might need to be
// as much as 150 system clock cycles, and perhaps longer if 
// another ISR preempts or delays the T1 ISR.
// 
// The following reserves a conservative 256 system clock cycles.
// Using a power of 2 guarantees that the expression always 
// evaluates to an integer. The (T1_CLOCK_FREQ / SYS_FREQ) factor 
// converts system clocks into T1 clocks.
//
//#define CO_MAX_RESERVE			(256 * T1_CLOCK_FREQ / SYS_FREQ)
//
#define CO_MAX_RESERVE			150		// T1 clocks, for minimum reserve,
										// assuming T1_CLOCK_FREQ == SYS_FREQ

#define CO_MAX					(CO_PERIOD * T1_CLOCK_FREQ / T0_FREQ - CO_MAX_RESERVE)
#if CO_MAX > TIMER_MAX
	#undef CO_MAX
	#define CO_MAX				TIMER_MAX
#endif

// If CO is less than CO_MIN_RESERVE, then the T1 isr can't turn it 
// off on schedule. Empirical timing tests found a minimum pulse 
// width of about 4 microseconds, which worked out to about 22 system
// clocks at a clock frequency of 5529600 Hz.
// ~4 us / 0.18 us / T1_CLOCK ~= 22
// As with CO_MAX_RESERVE, use a power of 2 and convert to T1 clocks.
//
//#define CO_MIN_RESERVE			(32 * T1_CLOCK_FREQ / SYS_FREQ)
//
#define CO_MIN_RESERVE			25		// T1 clocks, for minimum reserve,
										// assuming T1_CLOCK_FREQ == SYS_FREQ

#define CO_MIN					CO_MIN_RESERVE
#if CO_MIN < CO_MIN_RESERVE
	#undef CO_MIN
	#define CO_MIN				CO_MIN_RESERVE
#endif


#define CPW_MIN					5				// microseconds, based on CO_MIN_RESERVE
#define CPW_CTR					1500			// nominal center
//#define CPW_MAX					2995			// microseconds, based on equal about center
#define CPW_MAX					11851			// limited by CO_MAX: CPW_MAX = CO_MAX * 1000000.0 / T1_FREQ


///////////////////////////////////////////////////////
//
// global variables
//

volatile uint16_t T0Ticks;		// rolls over when max unsigned int is reached

#define ANALOG_INPUTS			2
uint8_t Ach[ANALOG_INPUTS] = { 1, 2 };		// SERVO_I = ANA1, SERVO_V = ANA2
int Ain[ANALOG_INPUTS];
#define AdcServoCurrent			Ain[0]
#define AdcServoVoltage			Ain[1]

volatile BOOL EnableControllerUpdate = TRUE;

volatile BOOL EnableDatalogging;
uint8_t DatalogCount;					// counter for Datalogging
uint8_t DatalogReset = -1;				// report every (this many + 1) seconds
	// Note: -1 == 0xFF == 255 is used as a disable value (DatalogReset is actually unsigned)
	// This is convenient because the reset value needs to be one 
	// less than the desired count.

uint16_t CO;							// this is the servo command signal
reentrant void (*do_CO)();				// a pointer to the function that produces the CO signal

int CommandedChannel;					// commanded channel

volatile uint8_t Channel;				// the selected channel (servo address)
uint16_t Cpw;							// control pulse width, in microseconds
uint16_t Co;							// control pulse width, T1 ticks
uint16_t Vps;							// SERVO_V, servo power supply voltage, millivolts

// reimplement as flags?
BOOL GoCommanded;						// a 'go' command was received
BOOL Stopped;							// a stop condition is met
BOOL CpEnabled;							// turns the control pulse train on or off
BOOL StopOnLimit0;						// stop when limit0 reached
BOOL Limit0;							// LIMIT0_DETECTED
BOOL StopOnLimit1;						// stop when limit1 reached
BOOL Limit1;							// LIMIT1_DETECTED

uint16_t StopOnMilliamps;				// stop if current exceeds this many milliamps
uint16_t Milliamps;						// SERVO_I, actuator motor current in milliamps
uint16_t StopOnTimeout;					// stop after this many hundredths of a second
volatile uint16_t Elapsed;				// 100ths of a second since servo started
#define ELAPSED_RESET			32767

//////////////////////////////////////////////////////
//
// internal prototypes
//
///////////////////////////////////////////////////////
void isr_timer0();
void isr_timer1();
void isr_adc();
reentrant void doNothing();
void setCpw(int);


///////////////////////////////////////////////////////
// set defaults
void init_irq()
{
	SET_VECTOR(TIMER0, isr_timer0);
	EI_T0();

	SET_VECTOR(TIMER1, isr_timer1);
	EI_T1();
	
	ADC_SELECT(Ach[0]);
	SET_VECTOR(ADC, isr_adc);
	EI_ADC();
	
	do_CO = doNothing;
	
	CommandedChannel = CHANNEL_NONE;
	Channel = 0;		// != CommandedChannel, to force a selection before doing anything
	setCpw(CPW_CTR);

	GoCommanded = FALSE;
	Stopped = TRUE;
	CpEnabled = FALSE;
	StopOnLimit0 = TRUE;
	Limit0 = FALSE;
	StopOnLimit1 = TRUE;
	Limit1 = FALSE;
	
	StopOnMilliamps = 0;
	Milliamps = 0;
	StopOnTimeout = 0;
	Elapsed = 0;
	Error = ERROR_NONE;	
}



///////////////////////////////////////////////////////
void preset() { }


///////////////////////////////////////////////////////
BOOL uint8CounterReset(uint8_t *count, uint8_t reset)
{
	if (*count == reset)
	{
		*count = 0;
		return TRUE;
	}
	++*count;
	return FALSE;
}


///////////////////////////////////////////////////////
BOOL uint16CounterReset(uint16_t *count, uint16_t reset)
{
	if (*count == reset)
	{
		*count = 0;
		return TRUE;
	}
	++*count;
	return FALSE;
}



///////////////////////////////////////////////////////
reentrant void doNothing() {}


///////////////////////////////////////////////////////
reentrant void selectCommandedChannel()
{
	uint8_t ta, tb, tc;	// workspaces for bit manipulations
	ta = PAIN & 0xF1;	// clear ADDR bits in PA (xxxx210x)
	tb = PBIN & 0xFE;	// clear ADDR bits in PB (xxxxxxx3)
	tc = PCIN & 0xF3;	// clear ADDR bits in PC (xxxx45xx)

	mask_set(ta, (CommandedChannel & 0x07) << 1);	// select and move PA bits of ADDR (xxxxx321)
	mask_set(tb, (CommandedChannel & 0x08) >> 3);	// select and move PB bits of ADDR (xxxx0xxx)
	mask_set(tc, ((CommandedChannel & 0x20) >> 3) | ((CommandedChannel & 0x10) >> 1) );  	// select and move PC bits of ADDR (xx23xxxx)

	// SERVO_CP must be low when changing channel to minimize twitching
	SERVO_CP_low();
	PAOUT = ta;
	PBOUT = tb;
	PCOUT = tc;
	SERVO_CP_high();

	Channel = CommandedChannel;
}


reentrant void outputCP()
{
	set_timer1_mark(CO);			// set the stop time
	SERVO_CP_high();				// start the pulse
	start_timer1();					// timer1 ISR stops the pulse
}


///////////////////////////////////////////////////////
void update_CO()
{
	reentrant void (*t)();

	if (Channel != CommandedChannel)
		t = selectCommandedChannel;
	else if (CpEnabled && (CO = Co) != 0)
		t = outputCP;
	else
		t = doNothing;
	
	DI();
	do_CO = t;
	EI();
}


///////////////////////////////////////////////////////
void update_device()
{
	// update device state
	Limit0 = LIMIT0_detected();
	Limit1 = LIMIT1_detected();
	Milliamps = A1_GAIN * (AdcServoCurrent - A1_OFFSET);
	Vps = A2_GAIN * (AdcServoVoltage - A2_OFFSET);

	// check for error conditions
	if (Vps < V_MIN)		mask_set(Error, ERROR_LOW_POWER);
	else					mask_clr(Error, ERROR_LOW_POWER);

	// why is this (necessarily) an error?
	if (Limit0 && Limit1)	mask_set(Error, ERROR_BOTH_LIMITS);
	else					mask_clr(Error, ERROR_BOTH_LIMITS);
	
	// check for stop conditions
	if 
		(
			(StopOnLimit0 && Limit0) ||
			(StopOnLimit1 && Limit1) ||
			(StopOnMilliamps > 0 && Elapsed > SKIP_INRUSH && Milliamps > StopOnMilliamps) ||
			(StopOnTimeout > 0 && Elapsed >= StopOnTimeout)
		)
	{
		Stopped = TRUE;
		CpEnabled = FALSE;
	}
	else
		Stopped = FALSE;

	if (GoCommanded)
	{
		if (!Stopped) CpEnabled = TRUE;
		GoCommanded = FALSE;
	}	
}


///////////////////////////////////////////////////////
void check_adc()
{
	static uint8_t achIndex;
	int adcd;
	if (AdcdReads < 17) return;
	
	adcd = Adcd;
	if (ADCD_VALID(adcd))
		adcd = (adcd >> 3) - ADC_OFFSET;
	else
		adcd = ADC_OUTOFRANGE;
	Ain[achIndex] = adcd;
	uint8CounterReset(&achIndex, ANALOG_INPUTS-1);
	ADC_SELECT(Ach[achIndex]);
	adc_reset();
}


///////////////////////////////////////////////////////
void update_controller()
{
	check_adc();
	if (EnableControllerUpdate)
	{
		EnableControllerUpdate = FALSE;	// re-enabled later by isr_timer0

		update_device();
		update_CO();
	}	
}


///////////////////////////////////////////////////////
void report_header()
{
	printromstr(R"SRV __CPW G L0 L1 ILIM ___I __TLIM __ELAP _____V Error");
	endMessage();
}


////////////////////////////////////////////////////////
void report_device()
{
	printi(Channel, 3, ' '); printSpace();
	printi(Cpw, 5, ' '); printSpace();
	printi(CpEnabled, 1, ' '); printSpace();
	printi(StopOnLimit0, 1, ' ');
	printi(Limit0, 1, ' '); printSpace();
	printi(StopOnLimit1, 1, ' ');
	printi(Limit1, 1, ' '); printSpace();
	printi(StopOnMilliamps, 4, ' '); printSpace();
	printi(Milliamps, 4, ' '); printSpace();
	printdec(StopOnTimeout, 6, ' ', 2); printSpace();
	printdec(Elapsed, 6, ' ', 2); printSpace();
	printdec(Vps, 6, ' ', 3); printSpace();
	printi(Error, 5, ' ');
	endMessage();
}

void setCpw(int cpw)
{
	Cpw = cpw;
	Co = (float)cpw  * (T1_FREQ / 1000000.0);
}

///////////////////////////////////////////////////////
void do_commands()
{
	char c;
	int n;

	while (!RxbEmpty())				// process a command
	{
		c = getc();		
		mask_clr(Error, ERROR_COMMAND);
		
		// single-byte commands
		if (c == '\0')				// null command
		{
			// (treat as single-byte command that does nothing)
		}
		else if (c == 'z')			// program data
		{
			printromstr(FIRMWARE); printromstr(VERSION); endMessage();
			printromstr(R"S/N:"); printi(SERNO, 4, ' '); endMessage();
			printromstr(R"CPW_MIN:"); printi(CPW_MIN, 4, ' ');
			printromstr(R"   CPW_MAX:"); printi(CPW_MAX, 6, ' '); endMessage();
			endMessage();
		}
		else if (c == 'h')			// report header
		{
			report_header();
		}
		else if (c == 's')			// stop
		{
			CpEnabled = FALSE;
			GoCommanded = FALSE;
		}
		else if (c == '0')			// reset to center
		{
			setCpw(CPW_CTR);
		}
		else						// multi-byte command
		{
			getArgs();

			if (c == 'n')					// select channel
			{				
				CpEnabled = FALSE;		// disable output
				GoCommanded = FALSE;
				
				n = tryArg(0, CHANNELS - 1, ERROR_CHANNEL, Channel, FALSE);
				if (!(Error & ERROR_CHANNEL))
					CommandedChannel = n;
			}
			else if (c == 'r')				// report
			{
				if (argPresent())			// set Datalogging interval
				{
					// DatalogReset rolls under to 0xFF (meaning "disable") if command arg is 0
					DatalogReset = tryArg(0, 255, ERROR_DATALOG, DatalogReset + 1, FALSE) - 1;
					DatalogCount = 0;
				}
				else						// one-time report
					report_device();
			}
			else if (c == 'p')				// set control pulse width
			{
				setCpw(tryArg(CPW_MIN, CPW_MAX, ERROR_CPW, Cpw, FALSE));
			}
			else if (c == 'g')				// go
			{
				if (argPresent())			// it's control pulse width
					setCpw(tryArg(CPW_MIN, CPW_MAX, ERROR_CPW, Cpw, FALSE));
				Elapsed = 0;
				GoCommanded = TRUE;			
			}
			else if (c == 'i')				// set current limit
			{
				StopOnMilliamps = tryArg(0, CURRENT_MAX, ERROR_ILIM, StopOnMilliamps, FALSE);
			}
			else if (c == 't')				// set timeout				
			{
				StopOnTimeout = tryArg(0, TIMEOUT_MAX, ERROR_TIMEOUT, StopOnTimeout, TRUE);
			}
			else if (c == 'l')				// (letter 'l', not number '1') set stop limits
			{
				n = atoi();
				mask_clr(Error, ERROR_LIMSW);
				if (n == 10) {						//  10 == enable limit 0
					StopOnLimit0 = TRUE;
				} else if (n == 11) {				//  11 == enable limit 1
					StopOnLimit1 = TRUE;
				} else if (n == -10) {				// -10 == disable limit 0
					StopOnLimit0 = FALSE;
				} else if (n == -11) {				// -11 == disable limit 1
					StopOnLimit1 = FALSE;
				} else {
					mask_set(Error, ERROR_LIMSW);
				}
			}				
			else					// unrecognized command
			{
				mask_set(Error, ERROR_COMMAND);
			}
		}
	}
	
	if (EnableDatalogging)
	{
		EnableDatalogging = FALSE;	// re-enabled later by isr_timer0
		if (DatalogReset != 0xFF && uint8CounterReset(&DatalogCount, DatalogReset))
			report_device();
	}
}


///////////////////////////////////////////////////////
// 
#pragma interrupt
void interrupt isr_timer0()
{
	++T0Ticks;	

	#if CO_PERIOD > 1
		if (CO_INTERVAL)
	#endif
		 {
			stop_timer1();
			IRQ_CLEAR_T1();
			EI();
			do_CO();
		 }
	#if CO_PERIOD > 1
		 else
			EI(); 
	#endif
	
	#if SERVICE_PERIOD > 1
		if (SERVICE_INTERVAL)
	#endif
		{
			if (CpEnabled)
				Elapsed = (Elapsed + 1) & ELAPSED_RESET;
		}

	#if CU_PERIOD > 1
		if (CU_INTERVAL)
	#endif
		{
			EnableControllerUpdate = TRUE;
		}
				
	if (ONE_SECOND)
		EnableDatalogging = TRUE;
}


///////////////////////////////////////////////////////
// stop CO pulse
#pragma interrupt
void isr_timer1()
{
	SERVO_CP_low();
}


///////////////////////////////////////////////////////
// ADC read complete...
#pragma interrupt
void isr_adc()
{ 
	Adcd = ADCD;
	++AdcdReads;
}
