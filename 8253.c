#include "main.h"

/*
	Two clocks are used for 8253, that are 2 MHz and 31.25 Hz.
	There are 3 8253 timers:
		#0: 16 bit timer working at 2 MHz: emulated by PIC Timer3
		#1: 16 bit timer working at 31250 Hz: emulated by PIC Timer4 and DMA2
		#2: 16 bit timer working with the trigger by timer #1.

	To emulate 2 MHz, a prescaler of 1/16 is used, resulting in 3 MHz.
	The PR3 value is 3/2 of 8253 register value.
	Slowest frequency of sound is 22.5 Hz, which is lowest tone capable for human.
	On the other hand, 8253 counter value is read as 2/3 of TMR3 value.

	To emulate 31250 Hz, 1536 (=6*256) counts of 48 MHz timer is required.
	Use Timer 4 for this (PR4=1535). DMA2 is also used to emulate 8253 timer #1.
	Set CHSIRQ=19 for Timer4 interrupt. DCH2SSIZ is the N value of 8253 timer #1.
	The 8253 timer #1 counter value will be DCH2SSIZ - DCH2SPTR. 

	Modes of 8253 counters;
		#0: mode 2 for sound
		#1: mode 2 for clock of timer #2
			MODE 2: RATE GENERATOR
			This Mode functions like a divide-by-N counter. It is typically
			used to generate a Real Time Clock Interrupt. OUT will
			initially be high. When the initial count has decremented to 1,
			OUT goes low for one CLK pulse. OUT then goes high
			again, the Counter reloads the initial count and the process
			is repeated. Mode 2 is periodic; the same sequence is
			repeated indefinitely. For an initial count of N, the sequence
			repeats every N CLK cycles.

			When N is 3, counter changes as: 3, 2, 1, 3, 2, 1, 3, 2, 1 ...

		#2: mode 0 for interrupt
			MODE 0: INTERRUPT ON TERMINAL COUNT
			Mode 0 is typically used for event counting. After the Control
			Word is written, OUT is initially low, and will remain low until
			the Counter reaches zero. OUT then goes high and remains
			high until a new count or a new Mode 0 Control Word is
			written to the Counter.

			When N is set to 3, counter changes as: 3, 2, 1, 0, 0xffff, 0xfffe ...

	The control register is set to:
		0x34: Mode 2 for timer #0. Read / write least significant byte first, then most significant byte.
		0x74: Mode 2 for timer #1. Read / write least significant byte first, then most significant byte.
		0xB0: Mode 0 for timer #2. Read / write least significant byte first, then most significant byte.
		0x80: Mode 0 for timer #2. Counter latch command.
	Therefore, the modes of timers are fixed for these settings.

*/

static int g_dch2dst;
static unsigned int g_8253timer2;
unsigned char g_z80int;

static unsigned int g_cacheTimer0,g_cacheTimer1,g_cacheTimer2;
static unsigned char g_modeTimer0,g_modeTimer1,g_modeTimer2;

void init_8253(void){
	// Timer3 is used for 8253 timer #0
	T3CON=0;
	T3CONbits.TCKPS=4; // 1/16 prescaller

	// OC2 is connected to RB5 and controled by Timer3
	OC2CON=0;
	OC2CONbits.OCTSEL=1;  // Timer3 is the clock source
	OC2CONbits.OCM=3;     // Toggle mode
	OC2R=0;               // When TMR3=0
	OC2CONbits.ON=1;
	// OC2 output to RB5
	TRISBbits.TRISB5=0;
	RPB5R=5;
	
	// Timer4 is used for 8253 timer #1 and #2
	T4CON=0;
	PR4=1536-1; // 31250 Hz

	//DMA2 settings for 8253 timer #1
	DMACONSET=0x8000;
	DCH2CON=0x00000012;  // CHBUSY=0, CHCHNS=0, CHEN=0, CHAED=0, CHCHN=0, CHAEN=1, CHEDET=0, CHPRI=b10
	DCH2ECON=0x1310;     // CHAIRQ=0, CHSIRQ=19, CFORCE=0, CABRT=0, PATEN=0, SIRQEN=1, AIRQEN=0
	                     // CHSIRQ=19: Timer4 interrupt
	DCH2SSA=0x1D000000; // Dummy source
	DCH2DSA=((unsigned int)&g_dch2dst)&0x1fffffff; // Dummy destonation
	DCH2SSIZ=31250;
	DCH2DSIZ=1;
	DCH2CSIZ=1;
	DCH2INTCLR=0x00FF00FF;
	//DCH2CONSET=0x00000080;
	// Interrupt setting
	DCH2INTbits.CHSDIF=0;
	DCH2INTbits.CHSDIE=1;
	IPC10bits.DMA2IP=2;
	IPC10bits.DMA2IS=0;
	IFS1bits.DMA2IF=0;
	IEC1bits.DMA2IE=1;

	// Start timers
	T3CONbits.ON=0; // 8253 timer #0 is controled by gate
	T4CONbits.ON=1; // 8253 timer #1 and #2 is always active.
}

void __ISR(_DMA_2_VECTOR,IPL2SOFT) DMA2Handler(void){
	g_8253timer2--;
	if (0==g_8253timer2) {
		// Z80 interrupt
		g_z80int=1;
	}
	DCH2INTbits.CHSDIF=0;
	IFS1bits.DMA2IF=0;
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
					i32=PR3-TMR3;
					g_cacheTimer0=((i32*43690)>>16)+1; // 43690/65536 ~ 2/3
					g_modeTimer0=0x30;
					return g_cacheTimer0&0x00ff;
				case 0x30: // (LSB then) MSB
					g_modeTimer0=0xb0;
					return g_cacheTimer0>>8;
				default: // Illegal
					return 0xff;
			}
		case 1: // Timer 1
			switch(g_modeTimer1){
				case 0x10: // MSB only
					return (DCH2SSIZ - DCH2SPTR)>>8;
				case 0xa0: // LSB only
					return (DCH2SSIZ - DCH2SPTR)&0x00ff;
				case 0xb0: // LSB (then MSB)
					g_cacheTimer1=(DCH2SSIZ - DCH2SPTR);
					g_modeTimer1=0x30;
					return g_cacheTimer1&0x00ff;
				case 0x30: // (LSB then) MSB
					g_modeTimer1=0xb0;
					return g_cacheTimer1>>8;
				default: // Illegal
					return 0xff;
			}
		case 2: // Timer 2
			switch(g_modeTimer2){
				case 0x10: // MSB only
					return g_8253timer2>>8;
				case 0xa0: // LSB only
					return g_8253timer2&0x00ff;
				case 0xb0: // LSB (then MSB)
					g_cacheTimer2=g_8253timer2;
					g_modeTimer2=0x30;
					return g_cacheTimer2&0x00ff;
				case 0x30: // (LSB then) MSB
					g_modeTimer2=0xb0;
					return g_cacheTimer2>>8;
				default: // Illegal
					return 0xff;
			}
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
					i32=data;
					i32<<=8;
					i32+=i32>>1; // 3/2
					if (65535<i32) i32=65535;
					PR3=i32-1;
					return;
				case 0xa0: // LSB only
					i32=data;
					i32+=i32>>1; // 3/2
					PR3=i32-1;
					return;
				case 0xb0: // LSB (then MSB)
					g_modeTimer0=0x30;
					g_cacheTimer0=data;
					return;
				case 0x30: // (LSB then) MSB
					g_modeTimer0=0xb0;
					i32=data;
					i32<<=8;
					i32|=g_cacheTimer0;
					i32+=i32>>1; // 3/2
					if (65535<i32) i32=65535;
					PR3=i32-1;
				default: // Illegal
					return;
			}
		case 1: // Timer 1
			switch(g_modeTimer1){
				case 0x10: // MSB only
					DCH2CONCLR=0x00000080;
					DCH2SSIZ=data<<8;
					DCH2CONSET=0x00000080;
					return;
				case 0xa0: // LSB only
					DCH2CONCLR=0x00000080;
					DCH2SSIZ=data;
					DCH2CONSET=0x00000080;
					return;
				case 0xb0: // LSB (then MSB)
					g_modeTimer1=0x30;
					g_cacheTimer1=data;
					return;
				case 0x30: // (LSB then) MSB
					g_modeTimer1=0xb0;
					i32=data;
					i32<<=8;
					i32|=g_cacheTimer1;
					DCH2CONCLR=0x00000080;
					DCH2SSIZ=i32;
					DCH2CONSET=0x00000080;
				default: // Illegal
					return;
			}
		case 2: // Timer 2
			switch(g_modeTimer2){
				case 0x10: // MSB only
					g_8253timer2=data<<8;
					return;
				case 0xa0: // LSB only
					g_8253timer2=data;
					return;
				case 0xb0: // LSB (then MSB)
					g_modeTimer2=0x30;
					g_cacheTimer2=data;
					return;
				case 0x30: // (LSB then) MSB
					g_modeTimer2=0xb0;
					i32=data;
					i32<<=8;
					i32|=g_cacheTimer2;
					g_8253timer2=i32;
				default: // Illegal
					return;
			}
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
					return;
				case 1: // Counter #1
					if (i8) g_modeTimer1=i8;
					if (g_modeTimer1!=0x10) g_modeTimer1|=0x80;
					g_cacheTimer1=(DCH2SSIZ - DCH2SPTR);
					return;
				case 2: // Counter #2
					if (i8) g_modeTimer2=i8;
					if (g_modeTimer2!=0x10) g_modeTimer2|=0x80;
					g_cacheTimer2=g_8253timer2;
					return;
				default: // illegal
					return;
			}
	}
}

unsigned char readE008() {
	// Tempo: 30 Hz
	return (drawcount & 0x01);
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
