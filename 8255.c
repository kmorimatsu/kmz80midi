#include "main.h"

/*
	Timer 555 will work at 33554432/48000000 Hz (0.7 Hz).
	33554432 = 2^25.
*/

static unsigned char portC;
static unsigned char portA;

static unsigned int timer555zero;

unsigned char read8255(unsigned short addr){
	unsigned char data;
	int i,j;
	switch (addr&0x03) {
		case 0: // PORT A
			return portA;
		case 1: // PORT B
			// Prepare result from keymatrix[]
			switch(portA&15){
				case 0:
					i =(g_keymatrix[ 0]&0x01);
					i|=(g_keymatrix[ 2]&0x01)<<1;
					i|=(g_keymatrix[ 4]&0x01)<<2;
					i|=(g_keymatrix[ 6]&0x01)<<3;
					i|=(g_keymatrix[ 8]&0x01)<<4;
					i|=(g_keymatrix[10]&0x01)<<5;
					i|=(g_keymatrix[12]&0x01)<<6;
					i|=(g_keymatrix[14]&0x01)<<7;
					return i^0xff;
				case 1:
					i =(g_keymatrix[ 1]&0x01);
					i|=(g_keymatrix[ 3]&0x01)<<1;
					i|=(g_keymatrix[ 5]&0x01)<<2;
					i|=(g_keymatrix[ 7]&0x01)<<3;
					i|=(g_keymatrix[ 9]&0x01)<<4;
					i|=(g_keymatrix[11]&0x01)<<5;
					i|=(g_keymatrix[13]&0x01)<<6;
					i|=(g_keymatrix[15]&0x01)<<7;
					return i^0xff;
				case 2:
					i =(g_keymatrix[ 0]&0x02)>>1;
					i|=(g_keymatrix[ 2]&0x02);
					i|=(g_keymatrix[ 4]&0x02)<<1;
					i|=(g_keymatrix[ 6]&0x02)<<2;
					i|=(g_keymatrix[ 8]&0x02)<<3;
					i|=(g_keymatrix[10]&0x02)<<4;
					i|=(g_keymatrix[12]&0x02)<<5;
					i|=(g_keymatrix[14]&0x02)<<6;
					return i^0xff;
				case 3:
					i =(g_keymatrix[ 1]&0x02)>>1;
					i|=(g_keymatrix[ 3]&0x02);
					i|=(g_keymatrix[ 5]&0x02)<<1;
					i|=(g_keymatrix[ 7]&0x02)<<2;
					i|=(g_keymatrix[ 9]&0x02)<<3;
					i|=(g_keymatrix[11]&0x02)<<4;
					i|=(g_keymatrix[13]&0x02)<<5;
					i|=(g_keymatrix[15]&0x02)<<6;
					return i^0xff;
				case 4:
					i =(g_keymatrix[ 0]&0x04)>>2;
					i|=(g_keymatrix[ 2]&0x04)>>1;
					i|=(g_keymatrix[ 4]&0x04);
					i|=(g_keymatrix[ 6]&0x04)<<1;
					i|=(g_keymatrix[ 8]&0x04)<<2;
					i|=(g_keymatrix[10]&0x04)<<3;
					i|=(g_keymatrix[12]&0x04)<<4;
					i|=(g_keymatrix[14]&0x04)<<5;
					return i^0xff;
				case 5:
					i =(g_keymatrix[ 1]&0x04)>>2;
					i|=(g_keymatrix[ 3]&0x04)>>1;
					i|=(g_keymatrix[ 5]&0x04);
					i|=(g_keymatrix[ 7]&0x04)<<1;
					i|=(g_keymatrix[ 9]&0x04)<<2;
					i|=(g_keymatrix[11]&0x04)<<3;
					i|=(g_keymatrix[13]&0x04)<<4;
					i|=(g_keymatrix[15]&0x04)<<5;
					return i^0xff;
				case 6:
					i =(g_keymatrix[ 0]&0x08)>>3;
					i|=(g_keymatrix[ 2]&0x08)>>2;
					i|=(g_keymatrix[ 4]&0x08)>>1;
					i|=(g_keymatrix[ 6]&0x08);
					i|=(g_keymatrix[ 8]&0x08)<<1;
					i|=(g_keymatrix[10]&0x08)<<2;
					i|=(g_keymatrix[12]&0x08)<<3;
					i|=(g_keymatrix[14]&0x08)<<4;
					return i^0xff;
				case 7:
					i =(g_keymatrix[ 1]&0x08)>>3;
					i|=(g_keymatrix[ 3]&0x08)>>2;
					i|=(g_keymatrix[ 5]&0x08)>>1;
					i|=(g_keymatrix[ 7]&0x08);
					i|=(g_keymatrix[ 9]&0x08)<<1;
					i|=(g_keymatrix[11]&0x08)<<2;
					i|=(g_keymatrix[13]&0x08)<<3;
					i|=(g_keymatrix[15]&0x08)<<4;
					return i^0xff;
				case 8:
					i =(g_keymatrix[ 0]&0x10)>>4;
					i|=(g_keymatrix[ 2]&0x10)>>3;
					i|=(g_keymatrix[ 4]&0x10)>>2;
					i|=(g_keymatrix[ 6]&0x10)>>1;
					i|=(g_keymatrix[ 8]&0x10);
					i|=(g_keymatrix[10]&0x10)<<1;
					i|=(g_keymatrix[12]&0x10)<<2;
					i|=(g_keymatrix[14]&0x10)<<3;
					return i^0xff;
				case 9:
					i =(g_keymatrix[ 1]&0x10)>>4;
					i|=(g_keymatrix[ 3]&0x10)>>3;
					i|=(g_keymatrix[ 5]&0x10)>>2;
					i|=(g_keymatrix[ 7]&0x10)>>1;
					i|=(g_keymatrix[ 9]&0x10);
					i|=(g_keymatrix[11]&0x10)<<1;
					i|=(g_keymatrix[13]&0x10)<<2;
					i|=(g_keymatrix[15]&0x10)<<3;
					return i^0xff;
				default:
					return 0xff;
			}
		case 2: // PORT C
			// C0-C3: ourput ports
			data=0;
			// C4: Motor ON/OFF (always 0)
			data|=0;
			// C5: CMT Read (always 0)
			data|=0;
			// C6: 555 out (0.7 Hz)
			if (0==portA&0x80) timer555zero=coretimer();
			data|=((coretimer()-timer555zero)&(1<<24)) ? (1<<6):0;
			// C7: /V-Blank
			data|=(TMR2<1524) ? 0:(1<<7);
			// Preparation of data is done
			return (portC&0x07) | data;
		default: // control
			return 0xff;
	}
}
void write8255(unsigned short addr, unsigned char data){
	switch (addr&0x03) {
		case 0: // PORT A
			if (data&0x80) {
				// RST=1 for 555 timer
				if (0==portA&0x80) timer555zero=coretime();
			} else {
				// RST=0 for 555 timer
				timer555zero=coretimer();
			}
			portA=data;
			return;
		case 1: // PORT B
			return;
		case 2: // PORT C
			// PC0: /V-GATE: if 0, do not show display
			g_video_disabled = (data&0x04) ? 0:1;
			// PC1: CMT write: ignored
			// PC2: LED Green/Red
			if (data&0x04) {
				led_red(0);
				led_green(1);
			} else {
				led_green(0);
				led_red(1);
			}
			// PC3: CLK for motor-flip-flop: ignored
			// Preparation of data is done
			portC=data;
			return;
		default: // control
			// portC-controling code will be here
			if (data&0x80) return; // Ignore configuration
			if (data&0x01) {
				// set bit
				data&=0x0e;
				data>>=1;
				data=1<<data;
				data|=portC;
				write8255(0x02,data);
			} else {
				// clear bit
				data&=0x0e;
				data>>=1;
				data=1<<data;
				data^=0xff;
				data&=portC;
				write8255(0x02,data);
			}
			return;
	}
}
