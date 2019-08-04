#define _SUPPRESS_PLIB_WARNING 1
#include "usbhost/FSIO.h"
#include "main.h"

extern volatile BOOL deviceAttached;
typedef struct{
	unsigned char  size;
	unsigned short address;
	unsigned char  type;
	unsigned char data[16];
} HEXLINE;

#define ERR_HEX_ERROR "Hex file error"
#define ERR_UNKNOWN "Unknown error"
#define ERR_FILEINVALID "File invalid"
#define ERR_USB "USB error"

// Result of reading a HEX file line
HEXLINE g_hexline;
FSFILE* g_fhandle;
char* g_fbuff;
int g_size;
int g_filepoint;
int g_srcpos;
unsigned int g_hexaddress;
// Vars used only in this file
static unsigned char g_checksum;

void hex_read_file(int blocklen){
	int i;
	if (blocklen==512) {
		// This is first read. Initialize parameter(s).
		g_srcpos=0;
		g_filepoint=0;
	} else if (g_size<512) {
		// Already reached the end of file.
		return;
	} else {
		// Shift buffer and source position 256 bytes.
		for(i=0;i<256;i++) g_fbuff[i]=g_fbuff[i+256];
		g_srcpos-=256;
		g_filepoint+=256;
	}
	// Read 512 or 256 bytes from SD card.
	g_size=512-blocklen+FSfread((void*)&g_fbuff[512-blocklen],1,blocklen,g_fhandle);
	// All lower cases
	for(i=512-blocklen;i<512;i++){
		if ('A'<=g_fbuff[i] && g_fbuff[i]<='F') g_fbuff[i]+=0x20;
	}
}

char* hex_init_file(char* buff,char* filename){
	// Detect USB memory
	if (!connect_usb()) return ERR_USB;

	// Open file
	g_fhandle=FSfopen(filename,"r");
	if (!g_fhandle) {
		return ERR_FILEINVALID;
	}
	// Initialize parameters
	g_fbuff=buff;
	g_srcpos=0;
	g_filepoint=0;
	// Read first 512 bytes
	hex_read_file(512);
	return 0;
}

int hex_read_byte(){
	unsigned char b1,b2;
	b1=g_fbuff[g_srcpos++];
	b2=g_fbuff[g_srcpos++];
	if ('0'<=b1 && b1<='9') {
		b1-='0';
	} else if ('a'<=b1 && b1<='f') {
		b1-='a';
		b1+=0x0a;
	} else {
		return -1;
	}
	if ('0'<=b2 && b2<='9') {
		b2-='0';
	} else if ('a'<=b2 && b2<='f') {
		b2-='a';
		b2+=0x0a;
	} else {
		return -1;
	}
	b1=(b1<<4)|b2;
	g_checksum+=b1;
	return b1;
}

char* hex_read_line(){
	int i,j;
	// Initialize checksum
	g_checksum=0;
	// Maintain at least 256 characters in cache.
	if (256<=g_srcpos) hex_read_file(256);
	// Read a hex file line
	if (g_fbuff[g_srcpos++]!=':') return ERR_HEX_ERROR;
	// Read size
	i=hex_read_byte();
	if (i<0) return ERR_HEX_ERROR;
	g_hexline.size=(unsigned char)i;
	// Read address
	i=hex_read_byte();
	if (i<0) return ERR_HEX_ERROR;
	g_hexline.address=(unsigned short)(i<<8);
	i=hex_read_byte();
	if (i<0) return ERR_HEX_ERROR;
	g_hexline.address|=(unsigned short)(i);
	// Ready type
	i=hex_read_byte();
	if (i<0) return ERR_HEX_ERROR;
	g_hexline.type=(unsigned char)i;
	// Read data
	for(j=0;j<g_hexline.size;j++){
		i=hex_read_byte();
		if (i<0) return ERR_HEX_ERROR;
		g_hexline.data[j]=(unsigned char)i;
	}
	// Read checksum
	i=hex_read_byte();
	if (i<0) return ERR_HEX_ERROR;
	if (g_checksum) return ERR_HEX_ERROR;
	// All done. Remove enter.
	if (g_fbuff[g_srcpos]=='\r') g_srcpos++;
	if (g_fbuff[g_srcpos]=='\n') g_srcpos++;
	return 0;
}

void stop_with_error(char* err){
	int i;
	// CLS
	for(i=0;i<1000;i++) VRAM[i]=0;
	// Start video again
	T2CONbits.ON=1;
	// Show error
	printstr(40,err);
	printstr(120,"Reset system to continue");
	// Infinite loop
	while(1){
		asm("wait");
	}
}

void write_hexline_to_ram(){
	int i;
	unsigned int addr=g_hexaddress+g_hexline.address;
	// Valid only in RAM address
	if (0x0000ffff<addr) return;
	if (addr<0x00002000) return;
	// Change 0x000xxxxx -> 0xa00xxxxx
	addr|=0xa0000000;
	// Write to RAM
	for(i=0;i<g_hexline.size;i++){
		((char*)addr)[i]=g_hexline.data[i];
	}
}

#define FWRITER_FILE_NAME "FWRITER.HEX"
#define RIGHT_ALLOW 0x40

int is_same_str(char* str1, char* str2){
	int i;
	for(i=0;str1[i]==str2[i];i++){
		if (0==str1[i]) return 1;
	}
	return 0;
}

void load_hexfile_and_run(char* filename){
	char* err;
	unsigned int i;
	static char* const appfilename=(char* const)0xA000FFF0;  
	// Open file
	err=hex_init_file(&VRAM2[0],FWRITER_FILE_NAME);
	if (err) stop_with_error(err);
	// Stop video
	T2CONbits.ON=0;
	// Check HEX file
	while(1){
		err=hex_read_line();
		if (err) return stop_with_error(err);
		if (g_hexline.type==1) {
			// EOF
			break;
		}
		if (g_size<=0) stop_with_error(ERR_HEX_ERROR);
	}
	FSrewind(g_fhandle);
	hex_read_file(512);
	// Read HEX and write to RAM
	while(1){
		err=hex_read_line();
		if (err) stop_with_error(err);
		if (g_hexline.type==1) {
			// EOF
			break;
		} else if (g_hexline.type==4) {
			// extended linear address
			i=g_hexline.data[0];
			i=i<<8;
			i|=g_hexline.data[0];
			i=i<<16;
			g_hexaddress=i;
		} else if (g_hexline.type==0) {
			// data
			write_hexline_to_ram();
		}
	}
	FSfclose(g_fhandle);
	// Set application file name
	for(i=0;appfilename[i]=filename[i];i++);
	// Char to ascii conversion
	for(i=0;appfilename[i];i++){
		appfilename[i]=char2ascii(appfilename[i]);
	}
	// Make RAM executable.
	mBMXSetRAMKernProgOffset(0x2000);
	mBMXSetRAMUserDataOffset(0x10000);
	mBMXSetRAMUserProgOffset(0x10000);
	// All done. Run code.
	asm volatile("di");
	asm volatile("ehb");
	asm volatile("la $v0,%0"::"i"(0xa0003000));
	asm volatile("jr $v0");
}

void file_select(void){
	SearchRec sr;
	int filenum,cursor,i;
	// Detect USB memory
	if (!connect_usb()) return;
	// Print header
	printstr(0,"MachiKania Boot: Move \x22\x20\x22 and Push CR");
	VRAM[23]=RIGHT_ALLOW;
	// Directory listing
   	filenum=0;
	cursor=121;
	if(FindFirst("*.hex",ATTR_MASK,&sr)){
		printstr(120,"No HEX File Found");
		while(1) asm volatile("wait");
	} else {
		do{
			if (is_same_str(sr.filename,FWRITER_FILE_NAME)) continue;
			printstr(cursor,sr.filename);
			filenum++;
			cursor+=13;
			if (0==(filenum % 3)) cursor++;
		} while(!FindNext(&sr) && filenum<66);
	}

	// Select a file
	cursor=120;
	drawcount=0;
	while(deviceAttached){
		// Blink the cursor
		if (drawcount&16) VRAM[cursor]=0;
		else VRAM[cursor]=RIGHT_ALLOW;
		// Detect CR key
		if (g_keymatrix2[8]&(1<<4)) {
			load_hexfile_and_run(&VRAM[cursor+1]);
		}
		// Detect right/left key
		if (g_keymatrix2[8]&(1<<3)) {
			// Detect shift key
			if (g_keymatrix2[8]&((1<<0)|(1<<5))) {
				i=cursor-13;
				if ((i%40)==27) i--;
			} else {
				i=cursor+13;
				if ((i%40)==39) i++;
			}
			// Check if valid movement
			if (i<120) i=cursor;
			else if (1000<i) i=cursor;
			else if (0x00==VRAM[i+1]) i=cursor;
			// Refresh view
			VRAM[cursor]=0;
			cursor=i;
			VRAM[cursor]=RIGHT_ALLOW;
			drawcount=0;
			// Wait while key down
			while(g_keymatrix2[8]&(1<<3)){
				USBTasks();
			}
		}
		// Detect up/down key
		if (g_keymatrix2[9]&(1<<2)) {
			// Detect shift key
			if (g_keymatrix2[8]&((1<<0)|(1<<5))) {
				i=cursor-40;
			} else {
				i=cursor+40;
			}
			// Check if valid movement
			if (i<120) i=cursor;
			else if (1000<i) i=cursor;
			else if (0x00==VRAM[i+1]) i=cursor;
			// Refresh view
			VRAM[cursor]=0;
			cursor=i;
			VRAM[cursor]=RIGHT_ALLOW;
			drawcount=0;
			// Wait while key down
			while(g_keymatrix2[9]&(1<<2)){
				USBTasks();
			}
		}
		USBTasks();
	}
}