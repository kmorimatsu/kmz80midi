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
#define _getCode() readMemory(regPC++)

/*
	UINT8 getCode();
	This function is used more frequently in code than _getCode() macro,
	so defined as a function. Using _getCode() macro in the function is an idea.
*/
UINT8 getCode();

/*
	RAM and I/O interface functions follows.
*/

UINT8 readMemory(UINT16 addr);
void writeMemory(UINT16 addr, UINT8 data);
UINT8 readIO(UINT8 addrL, UINT8 addrH);
void writeIO(UINT8 addrL, UINT8 addrH, UINT8 data);

