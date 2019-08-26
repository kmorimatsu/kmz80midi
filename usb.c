/*
	This file was modified for KM-Z80 implementation,
	from the file provided by Microchip.
	USB_ApplicationEventHandler() is provided by Microchip.
	The other functions are written by Katsumi.
*/

/********************************************************************
 FileName:     main.c
 Dependencies: See INCLUDES section
 Processor:		PIC18, PIC24, and PIC32 USB Microcontrollers
 Hardware:		This demo is natively intended to be used on Microchip USB demo
 				boards supported by the MCHPFSUSB stack.  See release notes for
 				support matrix.  This demo can be modified for use on other hardware
 				platforms.
 Complier:  	Microchip C18 (for PIC18), C30 (for PIC24), C32 (for PIC32)
 Company:		Microchip Technology, Inc.

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the “CompanyE for its PIC® Microcontroller is intended and
 supplied to you, the Company’s customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN “AS ISECONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

********************************************************************
 File Description:

 Change History:
  Rev   Description
  1.0   Initial release
  2.1   Updated for simplicity and to use common
                     coding style
********************************************************************/

#include "main.h"
#include "usbhost/FSIO.h"

volatile BOOL deviceAttached;
unsigned char VRAM2[1000];
unsigned char g_filename[13];

/*
	This module needs heap area, >= 448 bytes. Using 512 bytes would be a good idea.
*/

void init_usb(void){
	int i;
	// Wait for PLL lock to stabilize
	while(!(OSCCON&0x0020));
    deviceAttached = FALSE;
    //Initialize the stack
    USBInitialize(0);
}

/****************************************************************************
  Function:
    BOOL USB_ApplicationEventHandler( BYTE address, USB_EVENT event,
                void *data, DWORD size )

  Summary:
    This is the application event handler.  It is called when the stack has
    an event that needs to be handled by the application layer rather than
    by the client driver.

  Description:
    This is the application event handler.  It is called when the stack has
    an event that needs to be handled by the application layer rather than
    by the client driver.  If the application is able to handle the event, it
    returns TRUE.  Otherwise, it returns FALSE.

  Precondition:
    None

  Parameters:
    BYTE address    - Address of device where event occurred
    USB_EVENT event - Identifies the event that occured
    void *data      - Pointer to event-specific data
    DWORD size      - Size of the event-specific data

  Return Values:
    TRUE    - The event was handled
    FALSE   - The event was not handled

  Remarks:
    The application may also implement an event handling routine if it
    requires knowledge of events.  To do so, it must implement a routine that
    matches this function signature and define the USB_HOST_APP_EVENT_HANDLER
    macro as the name of that function.
  ***************************************************************************/

BOOL USB_ApplicationEventHandler( BYTE address, USB_EVENT event, void *data, DWORD size )
{
    switch( event )
    {
        case EVENT_VBUS_REQUEST_POWER:
            // The data pointer points to a byte that represents the amount of power
            // requested in mA, divided by two.  If the device wants too much power,
            // we reject it.
            return TRUE;

        case EVENT_VBUS_RELEASE_POWER:
            // Turn off Vbus power.
            // The PIC24F with the Explorer 16 cannot turn off Vbus through software.

            //This means that the device was removed
            deviceAttached = FALSE;
            return TRUE;
            break;

        case EVENT_HUB_ATTACH:
            return TRUE;
            break;

        case EVENT_UNSUPPORTED_DEVICE:
            return TRUE;
            break;

        case EVENT_CANNOT_ENUMERATE:
            //UART2PrintString( "\r\n***** USB Error - cannot enumerate device *****\r\n" );
            return TRUE;
            break;

        case EVENT_CLIENT_INIT_ERROR:
            //UART2PrintString( "\r\n***** USB Error - client driver initialization error *****\r\n" );
            return TRUE;
            break;

        case EVENT_OUT_OF_MEMORY:
            //UART2PrintString( "\r\n***** USB Error - out of heap memory *****\r\n" );
            return TRUE;
            break;

        case EVENT_UNSPECIFIED_ERROR:   // This should never be generated.
            //UART2PrintString( "\r\n***** USB Error - unspecified *****\r\n" );
            return TRUE;
            break;

        default:
            break;
    }

    return FALSE;
}

int fileselect(int save){
	SearchRec sr;
	int i,cursor;
	printstr(960,"-----SELECT FILE AND HIT SPACE KEY.-----");
	// Directory listing
	cursor=1;
	if (!FindFirst("*.MZT",ATTR_MASK,&sr)) {
		do {
			printstr(cursor,(char*)&sr.filename[0]);
			cursor+=13;
			if ((cursor%40)==0) cursor++;
		} while (cursor<960 && !FindNext(&sr));
	}
	// Select a file
	cursor=0;
	drawcount=0;
	while(deviceAttached){
		// Blink the cursor
		if (drawcount&16) VRAM[cursor]=0;
		else VRAM[cursor]=0x40;
		// Detect Break key
		if (g_keymatrix2[9]&(1<<3)) {
			return -1;
		}
		// Detect Space key
		if (g_keymatrix2[9]&(1<<1)) {
			// Space detected.
			cursor++;
			// Character to ascii code conversion
			for(i=0;i<12;i++){
				if (VRAM[cursor+i]) g_filename[i]=char2ascii(VRAM[cursor+i]);
				else break;
			}
			g_filename[i]=0x00;
			return 1;	
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
			if (i<0) i=cursor;
			else if (960<=i) i=cursor;
			else if (0x00==VRAM[i+1]) i=cursor;
			// Refresh view
			VRAM[cursor]=0;
			cursor=i;
			VRAM[cursor]=0x40;
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
			if (i<0) i=cursor;
			else if (960<=i) i=cursor;
			else if (0x00==VRAM[i+1]) i=cursor;
			// Refresh view
			VRAM[cursor]=0;
			cursor=i;
			VRAM[cursor]=0x40;
			drawcount=0;
			// Wait while key down
			while(g_keymatrix2[9]&(1<<2)){
				USBTasks();
			}
		}
		USBTasks();
	}
	return 0;
}

void filelist_view(void){
	int i;
	int* vram=(int*)&VRAM[0];
	int* vram2=(int*)&VRAM2[0];
	for(i=0;i<250;i++) {
		vram2[i]=vram[i];
		vram[i]=0;
	}
}

void mz_view(void){
	int i;
	int* vram=(int*)&VRAM[0];
	int* vram2=(int*)&VRAM2[0];
	for(i=0;i<250;i++) {
		vram[i]=vram2[i];
	}
}

char connect_usb(void){
	// Detect USB memory
	drawcount=0;
	while(1){
		USBTasks();
		if (USBHostMSDSCSIMediaDetect()) {
			if (FSInit()) break;
			// USB memory format error
			return 0;
		} else if (120<drawcount) {
			// Time out
			return 0;
		}
	}
	deviceAttached=TRUE;
	return 1;
}

char try_usbmemory(unsigned short regPC){
	static FSFILE* handle;
	static int s_filepos;
	int i,len,pos;

	// Detect USB memory
	if (!connect_usb()) return 0;

	switch(regPC){
		case 0x0437: // Save header
			// Select file from list
			filelist_view();
			pos=fileselect(1);
			mz_view();
			if (pos<0) return -1; // Break key
			if (!pos) break;      // Use CMT
			// Open file
			handle = FSfopen(g_filename,"w");
			if (!handle) break;
			// Write header information (128 bytes)
			len = FSfwrite((void *)&RAM[0x00f0],1,128,handle);
			FSfclose(handle);
			if (len!=128) break;
			reset_g_timer1();
			return 1;
			break;
		case 0x0476: // Save body
			// Open file
			handle = FSfopen(g_filename,"a");
			if (!handle) break;
			// Determine size and address to read
			len=RAM[0x0102];
			len+=RAM[0x0103]<<8;
			pos=RAM[0x0104];
			pos+=RAM[0x0105]<<8;
			pos-=0x1000;
			while (0<len) {
				if (512<len) {
					i = FSfwrite((void *)&RAM[pos],1,512,handle);
					if (i!=512) {
						FSfclose(handle);
						break;
					}
					pos+=512;
					len-=512;
				} else {
					i = FSfwrite((void *)&RAM[pos],1,len,handle);
					FSfclose(handle);
					if (i!=len) break;
					reset_g_timer1();
					return 1;
				}
			}
			break;
		case 0x04d9: // Load header
			// Select file from list
			filelist_view();
			pos=fileselect(0);
			mz_view();
			if (pos<0) return -1; // Break key
			if (!pos) break;      // Use CMT
			// Open file
			handle = FSfopen(g_filename,"r");
			if (!handle) break;
			// Read header information (128 bytes)
			len = FSfread((void *)&RAM[0x00f0],1,128,handle);
			FSfclose(handle);
			s_filepos=len;
			if (len!=128) break;
			// Store header information to RAM.
			reset_g_timer1();
			return 1;
		case 0x04f9: // Load body
			// Open file
			handle = FSfopen(g_filename,"r");
			if (!handle) break;
			// Skip header information (128 bytes)
			// If loading is done several times, skip to the next position
			if (FSfseek(handle,s_filepos,SEEK_SET)) {
				FSfclose(handle);
				break;
			}
			// Determine size and address to store
			len=RAM[0x0102];
			len+=RAM[0x0103]<<8;
			s_filepos+=len;
			pos=RAM[0x0104];
			pos+=RAM[0x0105]<<8;
			pos-=0x1000;
			while (0<len) {
				if (512<len) {
					i = FSfread((void *)&RAM[pos],1,512,handle);
					if (i!=512) {
						FSfclose(handle);
						break;
					}
					pos+=512;
					len-=512;
				} else {
					i = FSfread((void *)&RAM[pos],1,len,handle);
					FSfclose(handle);
					if (i!=len) break;
					reset_g_timer1();
					return 1;
				}
			}
			break;
		case 0x0589: // Verify body
			break;
		default:
			break;
	}
	reset_g_timer1();
	return 0;
}
