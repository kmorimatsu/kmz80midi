/*
   This file is provided under the MachiKania license,
   but not the LGPL license.
   Written by Katsumi.
   https://github.com/kmorimatsu/
*/

#include "main.h"
#include "cgrom.h"

/*
 * Build option must be "-g -O1 -G0" for not using $gp.
 * This will be required in interrupt-containing C-file in the future.
 * As making the video signal is a complicated process, it's good to think about this now.
 */

unsigned short drawcount;
unsigned short g_vline,g_keypoint;
unsigned char VRAM[1024];
unsigned char g_font[256*8];
int g_spibuffpoint;
unsigned int g_spibuff_w[10];
unsigned char* g_spibuff=(unsigned char*)&g_spibuff_w[0];
unsigned short g_keybuff[32];
unsigned char g_keymatrix[16];
unsigned char g_video_disabled;
#define START_SPI_SIGNAL (754-24-5)

void ntsc_init(void){
	int i;

	// Copy font from original
	for(i=0;i<256*8;i++) g_font[i]=cgrom[i];

	// Test message
	for(i=0;VRAM[i]="HELLO\x30WORLD\xa1@"[i]-0x40;i++);
	for(i=40;i<80;i++) VRAM[i]=0x44;
	for(i=960;i<1000;i++) VRAM[i]=0x44;
	for(i=0;i<10;i++){
		VRAM[80+i]=i+0x20;
		VRAM[90+i]=i+0x20;
		VRAM[100+i]=i+0x20;
		VRAM[110+i]=i+0x20;
	}
	printstr(120,"Hello World! #2");
	printhex32(160,0xabcdef01);

	// RB13 and RB15 are output pins
	TRISBbits.TRISB13=0; // SPI1
	TRISBbits.TRISB15=0; // OC1

	// SPI1 module settings follow
	SPI1CON=0;
	SPI1CONbits.ENHBUF=1;  // Enhanced Buffer Enable
	SPI1CONbits.MODE32=1;  // 32 bit mode
	SPI1CONbits.STXISEL=1; // SPI_TBE_EVENT is set when the buffer is completely empty
	SPI1CONbits.MSTEN=1;   // Master mode
	SPI1CON2=0x0300;       // All extended setting are off
	SPI1BRG=2;             // SPI clock=REFCLK/(2x(SPI2BRG+1))=8 Mhz.
	// Interrupt settings
	IPC7bits.SPI1IP=7;
	IPC7bits.SPI1IS=0;
	IFS1bits.SPI1TXIF=0;
	IEC1bits.SPI1TXIE=1;
	// Output SDO1 to RB13
	RPB13R=0x03;

	// OC1 module settings follow
	OC1CON=0;
	OC1CONbits.OCTSEL=0; // Timer 2 is used
	OC1CONbits.OCM=5;    // Dual, Continuous Output Pulse mode
	OC1RS=0;             // Output L when TMR2==0
	OC1R=110;            // Output H when TMR2==110 (see sync signal table)
	// Output OC1 to RB15
	RPB15R=0x05;

	// Timer2 is used for sync signal
	// Off, Not stop in Idle Mode, PBCLK, 1:1 prescale, 16-bit mode
	// Enable interrupt as priority 7
	// Either 1523 or 3047 for PR2 (see interrupt function)
	TMR2=0;
	PR2=110+1414-1;
	T2CON=0x0000;
	// Interrupt settings
	IPC2bits.T2IP=7;
	IPC2bits.T2IS=0;
	IFS0bits.T2IF=0;
	IEC0bits.T2IE=1;

	// Keyboard input settings follow
	// Output ports for RA5, RB7, RB8, and RB9
	TRISBSET=0x0380;
	TRISASET=0x10;
	LATBCLR=0x0380;
	LATACLR=0x10;
	// Input ports from RA0, RA1, RB2 and RB3
	TRISASET=0x03;
	TRISBSET=0x000c;
	// A/D converter settings
	AD1CON1=0x00E0;
	AD1CON2=0x0000;
	AD1CON3=0x1fff; // SAMC=31, TAD=10.8 usec (slowest mode)
	// CH0NA=0, CH0SA=0 (AN0, first)
	AD1CHS=0;

	// All ON
	SPI1CONbits.ON=1;
	OC1CONbits.ON=1;
	T2CONbits.ON=1;
	AD1CON1bits.ON=1;
	g_video_disabled=0;
}

/*
 * Equalizing pulses 3 times
 *  110 ____________ 110 ____________
 * |___|    1414    |___|    1414
 * 
 * Serrated pulses 3 times
 *      1298     ___     1298     ___
 * |____________|226|____________|226
 * 
 * Equalizing pulses 3 times
 *  110 ____________ 110 ____________
 * |___|    1414    |___|    1414
 * 
 * H-sync and videO signal 253 times
 *                            302  +-- Start SPI signal (TMR2 == 754)
 *  226 clocks              clocks |
 *  (4.7 usec) ____________XXXXXXXXvXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX__________
 * |__________| 226 clocks                 2524 clocks              72 clocks
 * 
 * The average frequence of sync signal is 16289 Hz.
 *
 * OC1 is used to make sync signal.
 * SPI1 is used to make video signal.
 * 
 * Fsck = Fpb / (2 x SPIxBRG + 1)
 * SPIxBRG = Fpb / Fsck / 2 - 1
 * Fsck=8 MHz, Fpb = 48 MHz
 * Therefore, SPIxBRG = 2
 * 
 * SPI 32 bits corresponds to 192 clocks.
 * Therefore, to start SPI signal at correct position, there are 4 ways:
 *   1) Start at 302 clocks after starting video signal (xxxx above)
 *   2) Start at 110 clocks after starting video signal; 0 in the beginning
 *   3) Start at 144 clocks after raising sync signal; 0,0 in the beginning
 *   4) Start at 178 clocks after dropping sync signal; 0,0,0 in the beginning
 * The method #1 is taken.
 * 
 * For 40 characters, 10 words (32 bits each) SPI signals are sent.
 * Total 12 words are sent. The SPI FIFO buffer can contain 4 words.
 * Therefore, sending SPI signals are done 3 times, 4, 3, and 3.
 */


#define read_ad_for_keymatrix(a) do {\
		x=ADC1BUF0;\
		g_keybuff[g_keypoint+a]=x;\
		y=g_keybuff[g_keypoint+16-a];\
		if (-20<(x-y) && (x-y)<20) {\
			x=(x+y)>>1;\
			g_keymatrix[g_keypoint]=(x>0x397)?0:((x>0x318)?16:((x>0x2d5)?8:((x>0x250)?4:((x>0x100)?2:1))));\
		}\
	} while(0) 	

// 3~4% of CPU resource will be taken by this interrupt when video signal isn't needed.
void __ISR(_TIMER_2_VECTOR,IPL7SOFT) T2Handler(void){
	asm volatile("#":::"v0");
	asm volatile("#":::"v1");
	int x,y;
	char* vram;
	char* font;
	const static short synctable[]={
		110,1414,110,1414,110,1414,110,1414,110,1414,110,1414, // lines 1-3
		1298,226,1298,226,1298,226,1298,226,1298,226,1298,226, // lines 4-6
		110,1414,110,1414,110,1414,110,1414,110,1414,110,1414, // lines 7-9
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 10-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 16-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 22-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 28-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 34-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 40-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 46-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 52-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 58-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 64-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 70-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 76-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 82-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 88-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 94-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 100-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 106-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 112-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 118-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 124-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 130-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 136-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 142-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 148-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 154-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 160-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 166-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 172-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 178-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 184-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 190-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 196-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 202-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 208-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 214-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 220-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 226-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 232-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 238-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 244-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 250-
		226,2822,226,2822,226,2822,226,2822,226,2822,226,2822, // lines 256-
		226,2822,                                              // line  262
	};
	static int synctable_point=0;
	IFS0bits.T2IF=0;
	// Refresh OC1RS and PR2
	OC1R=synctable[synctable_point++]-1;
	PR2=OC1R+synctable[synctable_point++];
	if (42*2+9*2<=synctable_point && synctable_point<=241*2+9*2) {
		// lines 42-241
		// Continue for NTSC video signal
		// TMR2=50~60 when reaching this line
		// Do something about key input here. Can use ~200 clocks for this purpose.
		if (synctable_point<(162*2+9*2)) {
			switch(synctable_point){
				case (42*2+9*2):
					// Read from RA0
					AD1CHSbits.CH0SA=0;
					AD1CON1CLR=1; // AD1CON1bits.DONE=0;
					AD1CON1SET=2; // AD1CON1bits.SAMP=1;
					break;
				case (66*2+9*2):
					// Read value
					read_ad_for_keymatrix(0);
					g_keypoint++;
					// Read from RA1
					AD1CHSbits.CH0SA=1;
					AD1CON1CLR=1; // AD1CON1bits.DONE=0;
					AD1CON1SET=2; // AD1CON1bits.SAMP=1;
					break;
				case (90*2+9*2):
					// Read value
					read_ad_for_keymatrix(0);
					g_keypoint++;
					// Read from RB2
					AD1CHSbits.CH0SA=4;
					AD1CON1CLR=1; // AD1CON1bits.DONE=0;
					AD1CON1SET=2; // AD1CON1bits.SAMP=1;
					break;
				case (114*2+9*2):
					// Read value
					read_ad_for_keymatrix(0);
					g_keypoint++;
					// Read from RB3
					AD1CHSbits.CH0SA=5;
					AD1CON1CLR=1; // AD1CON1bits.DONE=0;
					AD1CON1SET=2; // AD1CON1bits.SAMP=1;
					break;
				case (138*2+9*2):
					// Read value
					read_ad_for_keymatrix(0);
					g_keypoint-=3;
					// Read from RA0
					AD1CHSbits.CH0SA=0;
					AD1CON1CLR=1; // AD1CON1bits.DONE=0;
					AD1CON1SET=2; // AD1CON1bits.SAMP=1;
					break;
				default:
					break;
			}
		} else {
			switch(synctable_point){
				case (162*2+9*2):
					// Read value
					read_ad_for_keymatrix(16);
					g_keypoint++;
					// Read from RA1
					AD1CHSbits.CH0SA=1;
					AD1CON1CLR=1; // AD1CON1bits.DONE=0;
					AD1CON1SET=2; // AD1CON1bits.SAMP=1;
					break;
				case (186*2+9*2):
					// Read value
					read_ad_for_keymatrix(16);
					g_keypoint++;
					// Read from RB2
					AD1CHSbits.CH0SA=4;
					AD1CON1CLR=1; // AD1CON1bits.DONE=0;
					AD1CON1SET=2; // AD1CON1bits.SAMP=1;
					break;
				case (210*2+9*2):
					// Read value
					read_ad_for_keymatrix(16);
					g_keypoint++;
					// Read from RB3
					AD1CHSbits.CH0SA=5;
					AD1CON1CLR=1; // AD1CON1bits.DONE=0;
					AD1CON1SET=2; // AD1CON1bits.SAMP=1;
					break;
				case (234*2+9*2):
					// Read value
					read_ad_for_keymatrix(16);
					g_keypoint++;
					g_keypoint&=15;
					// Set next aquisition
					switch(g_keypoint>>2){
						case 0:
							TRISBSET=0x380; // RB7-RB9: off
							TRISACLR=0x10;  // RA4: on
							break;
						case 1:
							TRISASET=0x10;  // RA4: off
							TRISBCLR=0x80;  // RB7: on
							break;
						case 2:
							TRISBSET=0x380; // RB7-RB9: off
							TRISBCLR=0x100; // RB8: on
							break;
						default:
							TRISBSET=0x380; // RB7-RB9: off
							TRISBCLR=0x200; // RB9: on
							break;
					}
					break;
				default:
					break;
			}
		}	
		// Then return if video signal is not required.
		if (g_video_disabled) return;
		// Prepare g_spibuff ( 16+11*40+2 = 458 clocks )
		vram=&VRAM[(g_vline>>3)*40];
		font=&g_font[ 256*(g_vline & 7) ];
		for(x=0;x<40;x+=4){
			g_spibuff[x]  =font[vram[x+3]];
			g_spibuff[x+1]=font[vram[x+2]];
			g_spibuff[x+2]=font[vram[x+1]];
			g_spibuff[x+3]=font[vram[x]];
		}
		g_vline++;
		// TMR2=500~510 when reaching this line (clocks required for key input are ignored).
		// When using keyboard-reading routine, max value is 720
		// Wait until TMR2==START_SPI_SIGNAL
		do { } while(TMR2<START_SPI_SIGNAL);
		asm volatile("la $v0,%0"::"i"(&TMR2));
		asm volatile("lw $v0,0($v0)"); // less than 13 clocks taken after TMR2==START_SPI_SIGNAL
		asm volatile("addiu $v0,$v0,%0"::"i"(- START_SPI_SIGNAL));
		asm volatile("sll $v0,$v0,2");
		asm volatile("la $v1,label1");
		asm volatile("addu $v1,$v1,$v0");
		asm volatile("jr $v1");
		asm volatile("label1:");
		// 16 nop instructions (>13)
		asm volatile("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n");
		asm volatile("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n");
		// TMR2==START_SPI_SIGNAL - 24 when reaching this line
		// Send two words now. 5 clocks are required to set SPI1BUF.
		SPI1BUF=g_spibuff_w[0];
		SPI1BUF=g_spibuff_w[1];
		SPI1BUF=g_spibuff_w[2];
		SPI1BUF=g_spibuff_w[3];
		g_spibuffpoint=4;
		// Enable SPI1 interrupt
		IFS1bits.SPI1TXIF=0;
		IEC1bits.SPI1TXIE=1;
	} else if (sizeof(synctable)/sizeof(synctable[0])<=synctable_point) {
		// End of table
		drawcount++;
		// Reset parameters
		synctable_point=0;
		g_vline=0;
		// Raise CS0 interrupt every 60.1 Hz
		IFS0bits.CS0IF=1;
	}
}

/*
 * SPI1RXHanlder: Send SPI video signals
 * This function will be replaced by DMA in the future
 */
void __ISR(_SPI1_VECTOR, IPL7SOFT) SPI1RXHanlder(void){
	SPI1BUF=g_spibuff_w[g_spibuffpoint++];
	SPI1BUF=g_spibuff_w[g_spibuffpoint++];
	SPI1BUF=g_spibuff_w[g_spibuffpoint++];
	if (10<=g_spibuffpoint) IEC1bits.SPI1TXIE=0;
	IFS1bits.SPI1TXIF=0;
}

unsigned char ascii2char(unsigned char ascii){
	if (ascii<0x20 || 0x7f<ascii) return 0x00;
	return ascii2char_table[ascii-0x20];
}
void printstr(int cursor,char* str){
	int i;
	unsigned char c;
	for(i=0;c=str[i];i++) VRAM[cursor+i]=ascii2char(c);
}
void printhex4(int cursor, unsigned char val){
	if (val<10) VRAM[cursor]=val+0x20;
	else VRAM[cursor]=val-0x09;
}
void printhex8(int cursor, unsigned char val){
	printhex4(cursor,val>>4);
	printhex4(cursor+1,val&15);
}
void printhex16(int cursor, unsigned short val){
	printhex8(cursor,val>>8);
	printhex8(cursor+2,val&255);
}
void printhex32(int cursor, unsigned int val){
	printhex16(cursor,val>>16);
	printhex16(cursor+4,val&65535);
}
