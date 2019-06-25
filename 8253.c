#include "main.h"

/*
	Two clocks are used for 8253, that are 2 MHz and 31.25 Hz.
	There are 2 timers:
		16 bit timer working at 2 MHz: emulated by Timer3
		32 bit timer working at 31.25 Hz: emulated by Timer5

	To emulate 2 MHz, a prescaler of 1/32 is used, resulting in 1.5 MHz.
	The PR3 value is 3/4 of 8253 register value.
	On the other hand, when 8253 counter value is read as 4/3 of TMR3 value.

	To emulate 31.25 Hz, 1536000 counts of 48 MHz timer is required.
	153600 = 6000 * 256, therefore prescaler = 1/256, PR5=5999.
*/

unsigned char read8253(unsigned short addr){ return 0; }
void write8253(unsigned short addr, unsigned char data){ }
