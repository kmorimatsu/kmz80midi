#include "main.h"

/*
	Two clocks are used for 8253, that are 2 MHz and 31.25 Hz.
	There are 3 8253 timers:
		#0: 16 bit timer working at 2 MHz: emulated by PIC Timer3
		#1: 16 bit timer working at 31250 Hz: emulated by PIC Timer5
		#2: 16 bit timer working with the trigger by timer #1.

	To emulate 2 MHz, a prescaler of 1/16 is used, resulting in 3 MHz.
	The PR3 value is 3/2 of 8253 register value.
	Solwest frequency of sound is 22.5 Hz, which is lowest tone capable for human.
	On the other hand, 8253 counter value is read as 2/3 of TMR3 value.

	To emulate 31250 Hz, 1536 (=6*256) counts of 48 MHz timer is required.
	This timer is frequently used to count 31250 to cause 1 Hz frequency for 8253 timer #2.
	So, emulation is targeted for this purpose.

	Use Timer 5 with 1/256 prescaler, PR5=3071 ( = 6*512 -1), and interrupt.
	The interrupt occurs at 61.035 Hz frequency.

	To simulate this, Timer5 will work with 1/256 prescaller. PR5=49151 ( = 6*8192 -1 ).
	g_timer5 is an unsigned int valiable.
	( TMR5/6 ) is the bits 0-12 of 8253 timer 2.
	The bits 0-2 of g_timer5 is bits 13-15 of 8253 timer #1.
	The bits 3-18 of g_timer5 is bits 0-15 of 8253 timer #2.

	Modes of 8253 counters;
		#0: mode 2 for sound
		#1: mode 2 for interrupt lower bits
		#2: mode 0 for interrupt higher bits
Mode 0 is used for the generation of accurate time delay under software control. 
In this mode, the counter will start counting from the initial COUNT value loaded 
into it, down to 0. Counting rate is equal to the input clock frequency.

The OUT pin is set low after the Control Word is written, and counting starts one 
clock cycle after the COUNT is programmed. OUT remains low until the counter reaches 
0, at which point OUT will be set high until the counter is reloaded or the Control 
Word is written. The counter wraps around to 0xFFFF internally and continues counting, 
but the OUT pin never changes again. The Gate signal should remain active high for 
normal counting. If Gate goes low, counting is suspended, and resumes when it goes 
high again.

The first byte of the new count when loaded in the count register, stops the previous count. 
*/
/* 8253 mode 0:
	Then the timer reach zero, it will be always 0xffff after this event.
	Therefore, the PR5 is fixed at 0xffff.
	Writing valut changes TMR5 instead of PR5.
*/

static unsigned int g_timer5;
unsigned char g_z80int;

static unsigned int g_cacheTimer0,g_cacheTimer1,g_cacheTimer2;
static unsigned char g_modeTimer0,g_modeTimer1,g_modeTimer2;

void init_8253(void){
	// Timer3 is used for 8253 timer #0
	T3CON=0;
	T3CONbits.TCKPS=4; // 1/16 prescaller
	
	// Timer5 is used for 8253 timer #1 and #2
	T5CON=0;
	T5CONbits.TCKPS=7; // 1/256 prescaller
	PR5=6*8192-1;
	// Interrupt settings
	IPC5bits.T5IP=3;
	IPC5bits.T5IS=0;
	IFS0bits.T5IF=0;
	IEC0bits.T5IE=1;

	// Start timers
	T3CONbits.ON=0; // 8253 timer #0 is controled by gate
	T5CONbits.ON=1; // 8253 timer #1 and #2 is always active.
}

void __ISR(_TIMER_5_VECTOR,IPL3SOFT) T5Handler(void){
	// Interrupt occurs at 3.8 Hz.
	g_timer5++;
	if ((1<<18) <= g_timer5) {
		// Z80 interrupt
		g_z80int=1;
		// Reset timer
		g_timer5=0;
	}
	IFS0bits.T5IF=0;
}

unsigned char read8253(unsigned short addr){
	unsigned long i32;
	switch(addr&3){
		case 0: // Timer 0
			switch(g_modeTimer0){
				case 0x10: // MSB only
					i32=PR3-TMR3;
					i32=((i32*43690)>>16)+1; // 43690/65536 ~ 2/3
					return i32>>8;
				case 0xa0: // LSB only
					i32=PR3-TMR3;
					i32=((i32*43690)>>16)+1; // 43690/65536 ~ 2/3
					return i32&0x00ff;
				case 0xb0: // LSB (then MSB)
					return g_cacheTimer0>>8;
				case 0x30: // (LSB then) MSB
					return g_cacheTimer0&0x00ff;
				default: // Illegal
					return 0xff;
			}
		case 1: // Timer 1

		case 2: // Timer 2

		default:
			return 0xff;
	}
}
void write8253(unsigned short addr, unsigned char data){
	unsigned long i32;
	unsigned short i16;
	unsigned char i8;
	switch(addr&3){
		case 0: // Timer 0
			/*	8253 mode 2:
				When the timer in 8253 is, for example, set to 5, the 8253 counter changes as follows:
				5, 4, 3, 2, 1, 5, 4, 3, 2, 1, 5, 4, 3, 2, 1, 5, 4, 3, 2, 1, ...
				OUT is L when counter=1, otherwise, it is H.
				On the other hand, when PR3 is, for example, set to 4, TMR3 changes as follows:
				0, 1, 2, 3, 4, 0, 1, 2, 3, 4, ...
				Therefore, PR3 will be set as follows:
				PR3=value-1;
				Counter can be read as follows:
				value=PR3-TMR3+1;
			*/
			switch(g_modeTimer0){
				case 0x10: // MSB only
					T3CONbits.ON=0;
					i32=PR3-TMR3;
					i32=((i32*43690)>>16)+1; // 43690/65536 ~ 2/3
					i32&=0x00ff;
					i32|=data<<8;
					i32+=i32>>1; // 3/2
					if (PR3+1<i32) i32=PR3+1;
					TMR3=PR3+1-i32;
					T3CONbits.ON=1;
					return;
				case 0xa0: // LSB only
					T3CONbits.ON=0;
					i32=PR3-TMR3;
					i32=((i32*43690)>>16)+1; // 43690/65536 ~ 2/3
					i32&=0xff00;
					i32|=data;
					i32+=i32>>1; // 3/2
					if (PR3+1<i32) i32=PR3+1;
					TMR3=PR3+1-i32;
					T3CONbits.ON=1;
					return;
				case 0xb0: // LSB (then MSB)
					g_modeTimer0=0x30;
					g_cacheTimer0=data;
					return;
				case 0x30: // (LSB then) MSB
					g_modeTimer0=0xb0;
					i32=(data<<8) | g_cacheTimer0;
					if (PR3+1<i32) i32=PR3+1;
					TMR3=PR3+1-i32;
				default: // Illegal
					return;
			}
		case 1: // Timer 1
			return;
		case 2: // Timer 2
			return;
		default: // control
			/*
				Control word format
				         +------- Binary counter 16-bits
				         | +----- Binary coded decimal (BCD) counter (4 decades)
				D0: BCD  0 1
				         +----------- Mode 0
				         | +--------- Mode 1
				         | | +------- Mode 2
				         | | | +----- Mode 3
				         | | | | +--- Mode 4
				         | | | | | +- Mode 5
				D1: M0   0 1 0 1 0 1
				D2: M1   0 0 1 1 0 0
				D3: M2   0 0 X X 1 1
				         +------- Counter latching
				         | +----- Read/write most significant byte only
				         | | +--- Read/write least significant byte only
				         | | | +- Read/write least significant byte first, then most significant byte
				D4: RW0  0 1 0 1
				D5: RW1  0 0 1 1
				         +------- Select counter 0
				         | +----- Select counter 1
				         | | +--- Select counter 2
				         | | | +- illegal
				D6: SC0  0 1 0 1
				D7: SC1  0 0 1 1

				D0-D4 are ignored in this simulation
			*/
			/*
				D0: always 0
				D1: always 0
				D2: always 0
				D3: always 0
				g_modeTimerX values:
				         +------- Illegal
				         | +----- Read/write most significant byte only
				         | | +--- Read/write least significant byte only
				         | | | +- Read/write least significant byte first, then most significant byte
				D4: RW0  0 1 0 1
				D5: RW1  0 0 1 1
				D6: always 0
				         +------- Read/write most significant byte
				         | +----- Read/write least significant byte
				D7: LSB  0 1

TODO: around here
			*/
			switch(data&0x30){ // i8 is the mode
				case 0x10: i8=0x10; break;
				case 0x20: i8=0xa0; break;
				case 0x30: i8=0xb0; break;
				default:   i8=0; break;			
			}
			switch(data>>6){
				case 0: // Counter #0
					if (i8) g_modeTimer0=i8;
					if (g_modeTimer0!=0x10) g_modeTimer0|=0x80;
					i32=PR3-TMR3;
					g_cacheTimer0=((i32*43690)>>16)+1; // 43690/65536 ~ 2/3
					break;
				case 1: // Counter #1
					if (i8) g_modeTimer1=i8;
					if (g_modeTimer1!=0x10) g_modeTimer1|=0x80;
				case 2: // Counter #2
					if (i8) g_modeTimer0=i8;
					if (g_modeTimer1!=0x10) g_modeTimer1|=0x80;
				default: // illegal
					return;
			}
	}
}

unsigned char readE008() {
	// Tempo: 30 Hz
	return drawcount & 0x01;
}

void writeE008(unsigned char data) {
	if (data&0x01) {
		// Start sound
		T3CONbits.ON=1;
	} else {
		// Stop sound
		T3CONbits.ON=0;
	}
}
