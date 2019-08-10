/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   https://github.com/kmorimatsu/
*/

#include <xc.h>
#include <sys/attribs.h>

extern int g_temp;

extern volatile unsigned short drawcount;
extern unsigned short g_keybuff[32];
extern unsigned char g_keymatrix[16];
extern unsigned char g_keymatrix2[10];
extern unsigned char g_video_disabled;
extern unsigned char g_vblank;
extern unsigned char* RAM;
extern unsigned char* VRAM;
extern unsigned char VRAM2[1000];

unsigned int coretimer(void);
void reset_g_timer1(void);
void led_green(void);
void led_red(void);

void video_init(void);
unsigned char char2ascii(unsigned char code);
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

void init_usb(void);
char connect_usb(void);
char try_usbmemory(unsigned short regPC);

void file_select(void);
