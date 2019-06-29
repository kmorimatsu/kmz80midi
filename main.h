/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   https://github.com/kmorimatsu/
*/

#include <xc.h>
#include <sys/attribs.h>

extern int g_temp;

extern unsigned short drawcount;
extern unsigned short g_keybuff[32];
extern unsigned char g_keymatrix[16];
extern unsigned char g_keymatrix2[10];
extern unsigned char g_video_disabled;

unsigned int coretimer();
void led_green(int on);
void led_red(int on);

void ntsc_init(void);
unsigned char ascii2char(unsigned char ascii);
void printstr(int cursor,char* str);
void printhex4(int cursor, unsigned char val);
void printhex8(int cursor, unsigned char val);
void printhex16(int cursor, unsigned short val);
void printhex32(int cursor, unsigned int val);

void loadTape();

unsigned char readE008();
void writeE008(unsigned char data);

unsigned char read8253(unsigned short addr);
void write8253(unsigned short addr, unsigned char data);
unsigned char read8255(unsigned short addr);
void write8253(unsigned short addr, unsigned char data);
