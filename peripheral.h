/*
   This program is provided under the LGPL license ver 2.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

/*
	Environment(machine)-specific basic functions follow 
*/


/*
	_getCode() macro (as inline function) that fetches a code using PC, and increment PC.
	This macro is used more frequently in time than getCode() function, 
	so faster code is required than getCode() function.
*/
extern unsigned char RAM[];
extern unsigned char VRAM[];
extern const unsigned char monitor[];
#define _getCode() (\
		(regPC<0x1000 ? monitor[regPC++] : \
		(regPC<0xd000 ? RAM[(regPC++)-0x1000] : \
		(regPC<0xe000 ? VRAM[(regPC++)&0x3ff] : 0xc7 )\
	)))

/*
	RAM and I/O interface functions follows.
*/

UINT8 readMemory(UINT16 addr);
void writeMemory(UINT16 addr, UINT8 data);
UINT8 readIO(UINT8 addrL, UINT8 addrH);
void writeIO(UINT8 addrL, UINT8 addrH, UINT8 data);

// Used to truck C3 command for CMT load/save from/to SD-card.
void codeC9();
#define PRECODEC3 \
	if (0x22<=regPC && regPC<=0x2e) {\
		if (try_usbmemory(regPC)) {\
			codeC9();\
			return;\
		}\
	}


