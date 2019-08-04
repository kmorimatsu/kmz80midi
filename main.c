/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   https://github.com/kmorimatsu/
*/

#include "./main.h"
#include "z80.h"
#include "peripheral.h"

// Note: 4 MHz crystal is used. CPU clock is 48 MHz.
#pragma config FUSBIDIO  = OFF          // Not using USBID (pin #14, RB5)
#pragma config FVBUSONIO = OFF          // Not using VBUSON (pin #25, RB14)
#pragma config DEBUG = OFF

#pragma config UPLLEN   = ON            // USB PLL Enabled
#pragma config FPLLMUL  = MUL_24        // PLL Multiplier
#pragma config UPLLIDIV = DIV_1         // USB PLL Input Divider
#pragma config FPLLIDIV = DIV_1         // PLL Input Divider
#pragma config FPLLODIV = DIV_2         // PLL Output Divider
#pragma config FPBDIV   = DIV_1         // Peripheral Clock divisor
#pragma config FWDTEN   = OFF           // Watchdog Timer
#pragma config WDTPS    = PS1           // Watchdog Timer Postscale
//#pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
#pragma config OSCIOFNC = OFF           // CLKO Enable
#pragma config POSCMOD  = HS            // Primary Oscillator
#pragma config IESO     = OFF           // Internal/External Switch-over
#pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable (KLO was off)
#pragma config FNOSC    = PRIPLL        // Oscillator Selection
#pragma config CP       = OFF           // Code Protect
#pragma config BWP      = OFF           // Boot Flash Write Protect
#pragma config PWP      = OFF           // Program Flash Write Protect
#pragma config ICESEL   = ICS_PGx1      // ICE/ICD Comm Channel Select
#pragma config JTAGEN   = OFF           // JTAG disabled

// Boot area assmbly. Jump to 0x9d000000 here.
volatile const unsigned int const __attribute__((address(0xBFC00000))) boot[]={
	0x3c089d00, // lui $t0,0x9d00
	0x01000008, // jr $to
	0x00000000, // nop
};

int g_temp=0;

unsigned int coretimer(void){
	// mfc0 v0,Count
	asm volatile("mfc0 $v0,$9");
}

void reset_g_timer1(void){
	g_timer1=coretimer();
}

void led_green(){
	LATBbits.LATB1=1;
	TRISBbits.TRISB1=0;
}

void led_red(){
	LATBbits.LATB1=0;
	TRISBbits.TRISB1=0;
}

void main(void){
	int i,j;

	// Enable interrupt
	INTEnableSystemMultiVectoredInt();

	// Initialize video and keyboard
	ntsc_init();
	// Initialize USB host
	init_usb();
	// Start bootloader if a button is pushed
	while(drawcount<5) asm volatile("wait");
	for(i-0;i<10;i++){
		if (g_keymatrix2[i]) file_select();
	}
	// Initialize timers
	init_8253();
	// Initialize Z80
	resetZ80();
	// Load KM-BASIC to RAM
	loadTape();
	// Initialize timer for Z80 emulation
	reset_g_timer1();
	// Main loop
	while(1){
		// Wait until next timing to execute Z80 code
		i=g_timer1;
		while( 0 < (int)( ((unsigned int)i) - ((unsigned int)coretimer()) ) ){
			//VRAM[0]=i;
			//VRAM[0]=DCH2SPTR;
		}
		// Now, execute the Z80 code.
		// Note that g_timer1 will increment due to each code's T cycles
		execZ80code();
		execZ80code();
		execZ80code();
		execZ80code();
		execZ80code();
	}
}

