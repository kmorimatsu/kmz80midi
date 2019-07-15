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
			i=portA&15;
			if (i<10) return g_keymatrix2[i]^0xff;
			return 0xff;
		case 2: // PORT C
			// C0-C3: ourput ports
			data=0;
			// C4: Motor ON/OFF (always 0)
			data|=0;
			// C5: CMT Read (always 0)
			data|=0;
			// C6: 555 out (0.7 Hz)
			if (0==portA&0x80) timer555zero=coretimer();
			data|=((coretimer()-timer555zero)&(1<<23)) ? (1<<6):0;
			// C7: /V-Blank
			data|=(PR2<1524) ? 0:(1<<7);
			// Preparation of data is done
			return (portC&0x0f) | data;
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
			g_video_disabled = (data&0x01) ? 0:1;
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
