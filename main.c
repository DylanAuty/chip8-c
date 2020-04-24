// main.c
// Contains the main loop for the chip-8 emulator.

#include <stdio.h>
#include <stdint.h>

int main(int argc, char* argv[]){

	// Define the RAM, 4095 (0xFFF) bytes long.
	uint8_t mem[0xfff];

	// Define the registers, 16 8 bit, 1 16 bit.
	uint8_t V[0xf];		// General purpose registers
	uint16_t I;			// A larger register

	// Define special registers
	uint16_t PC = 0x0000;		// Program counter
	uint8_t SP = 0x00;			// Stack pointer
	uint8_t DT = 0x00;			// Delay timer, counts down at 60Hz when nonzero. Stops at zero.
	uint8_t ST = 0x00;			// Sound timer, counts down at 60Hz when nonzero, with buzzer sounding throughout. Stops at zero.

	uint16_t stack[0xf];	// The stack, up to 0xf return addresses allowed.

	uint8_t disp[0x40][0x20];	// Structure for the screen. Each uint8_t is to be used as if it represents one bit only.

	// Test memory initialisation.
	mem[0x000] = 0x12;		// Jump to address 200.
	mem[0x001] = 0x00;		

	mem[0x200] = 0x30;		// Skip next instruction if V0 == 0x00.
	mem[0x201] = 0x00;

	mem[0x204] = 0x24;		// CALL 0x400
	mem[0x205] = 0x00;
	mem[0x400] = 0x00;		// RET
	mem[0x401] = 0xEE;

	mem[0x206] = 0x62;		// Set V2 and V3 to 13
	mem[0x207] = 0x13;
	mem[0x208] = 0x63;
	mem[0x209] = 0x13;

	mem[0x20A] = 0x52;		// SNE if v2 == v3;
	mem[0x20B] = 0x30;

	mem[0x20E] = 0x62;		// Set V2 and V3 to 14 and 15 respectively
	mem[0x20F] = 0x14;
	mem[0x210] = 0x63;
	mem[0x211] = 0x15;

	mem[0x212] = 0x52;		// SNE if v2 == v3;
	mem[0x213] = 0x30;

	// Variables for the main loop
	uint16_t raw = 0x0000;		// Stores value read in from memory
	uint16_t PC_next = PC;		// Assigned to PC at the end of the main loop.

	while(1){
		printf("Reading from address 0x%x\n", PC);
		// Do stuff
		raw = (uint16_t)(mem[PC] << 0x8) + (uint16_t)(mem[PC + 0x1]);
		printf("raw = 0x%x\n", raw);
		printf("SP = 0x%x\n", SP);
		printf("v2 = 0x%x, V3 = 0x%x\n", V[0x2], V[0x3]);

		switch(raw & 0xF000){
			case 0x0000:
				switch(raw & 0x00FF){
					case 0x00E0:
						printf("0x00E0 Not implemented, this is a NOP.");
						PC_next = PC + 0x2;
						break;
					case 0x00EE:	// RET, 0x00EE
						SP--;
						PC_next = stack[SP] + 0x02;
						break;
					default:
						printf("Unknown instruction, this is a NOP.");
						PC_next = PC + 0x2;
				}
				break;
			case 0x1000:	// JP, 0x1NNN
				PC_next = raw & 0x0FFF;
				break;
			case 0x2000:	// CALL, 0x2NNN
				stack[SP] = PC;
				SP++;
				PC_next = raw & 0x0FFF;
				break;
			case 0x3000:	// SE Vx, 0x3xkk
				if(V[(raw & 0x0F00) >> 0x8] == (raw & 0x00FF)){
					PC_next = PC + 0x4;
				} else{
					PC_next = PC + 0x2;
				}
				break;
			case 0x4000:	// SNE Vx, 0x4xkk
				if(V[(raw & 0x0F00) >> 0x8] != (raw & 0x00FF)){
					PC_next = PC + 0x4;
				} else{
					PC_next = PC + 0x2;
				}
				break;
			case 0x5000:	// SE Vx, Vy, 0x5xy0
				if(V[(raw & 0x0F00) >> 0x8] == V[(raw & 0x00F0) >> 0x4]){
					PC_next = PC + 0x4;
				} else{
					PC_next = PC + 0x2;
				}
				break;
			case 0x6000:	// LD Vx, byte, 0x6xkk
				V[(raw & 0x0F00) >> 0x8] = (uint8_t)(raw & 0x00FF);
				PC_next = PC + 0x2;
				break;
			default:
				printf("Default\n");
				PC_next = PC + 0x2;
				break;
		}

		// Move on
		PC = PC_next;
		getchar();
	}

	return 0;
}

