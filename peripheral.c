
#include "z80.h"
#include "peripheral.h"
#include "monitor.h"

// Full RAM implementation for MZ-80K
unsigned char RAM[1024*48];

UINT8 readMemory(UINT16 addr){
	if (addr<0x1000) return monitor[addr];
	if (addr<0xD000) return RAM[addr-0x1000];
	if (addr<0xE000) return VRAM[addr & 0x03FF];
	if (addr<0xE004) return read8255(addr);
	if (addr<0xE008) return read8253(addr);
	if (addr<0xE009) return readE008();
	return 0xC7; // RST00
}

void writeMemory(UINT16 addr, UINT8 data){
	if (addr<0x1000) return; // ROM region cannot be written.
	if (addr<0xD000) {
		RAM[addr-0x1000] = data;
		return;
	}
	if (addr<0xE000) {
		VRAM[addr & 0x03FF] = data;
		return;
	}
	if (addr<0xE004) {
		write8255(addr,data);
		return;
	}
	if (addr<0xE008) {
		write8253(addr,data);
		return;
	}
	if (addr<0xE009) {
		writeE008(data);
		return;
	}
}

// MZ-80K does not use I/O-mapped I/O
UINT8 readIO(UINT8 addrL, UINT8 addrH){ return 0xff;}
void writeIO(UINT8 addrL, UINT8 addrH, UINT8 data){ }
