/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   https://github.com/kmorimatsu/
*/

#include "./main.h"

// Note: 4 MHz crystal is used. CPU clock is 48 MHz.
#pragma config FUSBIDIO  = OFF          // Not using USBID (pin #14, RB5)
#pragma config FVBUSONIO = OFF          // Not using VBUSON (pin #25, RB14)
#pragma config DEBUG = ON

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

int g_temp=0;

int coretime(){
	// mfc0 v0,Count
	asm volatile("mfc0 $v0,$9");
}

void led_green(int on){
	LATBbits.LATB0=on?0:1;
	TRISBbits.TRISB0=0;
}

void led_red(int on){
	LATBbits.LATB1=on?0:1;
	TRISBbits.TRISB1=0;
}

extern unsigned short g_keybuff[32];
extern unsigned char g_keymatrix[16];

void main(void){
	volatile int i,j;

	// Enable interrupt
	INTEnableSystemMultiVectoredInt();

	ntsc_init();

	while(1){
		for(i=0;i<16;i++){
			printhex16(200+i*40,g_keybuff[i]);
			printhex16(208+i*40,g_keybuff[i+16]);
			printhex8(216+i*40,g_keymatrix[i]);
		}
	}
}
             