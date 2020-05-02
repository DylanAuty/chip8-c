// main.c
// Contains the main loop for the chip-8 emulator.

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>


uint8_t get_hex_char();


int main(int argc, char* argv[]) {

	// Define the RAM, 4095 (0xFFF) bytes long.
	uint8_t mem[0x1000] = {0x00};

	// Define the registers, 16 8 bit, 1 16 bit.
	uint8_t V[0x10] = {0x00};		// General purpose registers
	uint16_t I = 0x0000;			// A larger register

	// Define special registers
	uint16_t PC = 0x0000;			// Program counter
	uint8_t SP = 0x00;				// Stack pointer
	uint8_t DT = 0x00;				// Delay timer, counts down at 60Hz when nonzero. Stops at zero.
	uint8_t ST = 0x00;				// Sound timer, counts down at 60Hz when nonzero, with buzzer sounding throughout. Stops at zero.

	uint16_t stack[0x10] = {0x0000};	// The stack, up to 0xf return addresses allowed.

	//uint8_t disp[0x40][0x20];		// Structure for the screen. Each uint8_t is to be used as if it represents one bit only.

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

	mem[0x214] = 0x62;		// Set V2 and V3 to 0xFF and 0x2 respectively
	mem[0x215] = 0xFF;
	mem[0x216] = 0x63;
	mem[0x217] = 0x02;

	mem[0x218] = 0x82;		// ADD Vx, Vy. This should produce a carry and leave V2 = 0x1.
	mem[0x219] = 0x34;

	mem[0x21a] = 0x82;		// SHR Vx. Sets VF to LSB of Vx, divides Vx by 2, stores result in Vx.
	mem[0x21b] = 0x36;

	mem[0x21c] = 0x62;		// LD Vx, K
	mem[0x21d] = 0xBA;

	mem[0x21e] = 0xF2;		// LD B, Vx
	mem[0x21f] = 0x33;

	mem[0x220] = 0xF2;		// LD [I], Vx
	mem[0x221] = 0x55;

	// Variables for the main loop
	uint16_t raw = 0x0000;		// Stores value read in from memory
	uint16_t PC_next = PC;		// Assigned to PC at the end of the main loop.
	uint16_t temp = 0x0000;		// A 16 bit temporary variable. Used when doing 8 bit carry-based operations.
	uint16_t x = 0x0000;		// To be used as primary register index
	uint16_t y = 0x0000;		// To be used as secondary register index, where needed.
	time_t start_time = time(0);
	time_t time_difference = 0;
	int DT_temp = 0;
	int ST_temp = 0;

	DT = 0xFF;
	ST = 0x13;

	while(1) {
		// Handle timed counters
		time_difference = time(0) - start_time;
		if((time_difference) >= 1) {
			DT_temp = DT - time_difference;
			ST_temp = ST - time_difference;
			DT -= time_difference;
			ST -= time_difference;
			DT *= !(DT_temp < 0);
			ST *= !(ST_temp < 0);
			start_time = time(0);
		}

		printf("DT = %x, ST = %x\n", DT, ST);

		//printf("Reading from address 0x%x\n", PC);
		// Do stuff
		raw = (uint16_t)(mem[PC] << 0x8) + (uint16_t)(mem[PC + 0x1]);
		//printf("raw = 0x%x\n", raw);
		//printf("SP = 0x%x\n", SP);
		//printf("v2 = 0x%x, V3 = 0x%x\n", V[0x2], V[0x3]);
		//printf("vf = 0x%x\n", V[0xF]);

		switch(raw & 0xF000) {
			case 0x0000:
				switch(raw & 0x00FF) {
					case 0x00E0:
						printf("0x00E0 Not implemented, this is a NOP.\n");
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
				x = (raw & 0x0F00) >> 0x8;
				if(V[x] == (raw & 0x00FF)) {
					PC_next = PC + 0x4;
				} else {
					PC_next = PC + 0x2;
				}
				break;
			case 0x4000:	// SNE Vx, 0x4xkk
				x = (raw & 0x0F00) >> 0x8;
				if(V[x >> 0x8] != (raw & 0x00FF)) {
					PC_next = PC + 0x4;
				} else {
					PC_next = PC + 0x2;
				}
				break;
			case 0x5000:	// SE Vx, Vy, 0x5xy0
				x = (raw & 0x0F00) >> 0x8;
				y = (raw & 0x00F0) >> 0x4;
				if(V[x] == V[y]) {
					PC_next = PC + 0x4;
				} else {
					PC_next = PC + 0x2;
				}
				break;
			case 0x6000:	// LD Vx, byte, 0x6xkk
				x = (raw & 0x0F00) >> 0x8;
				V[x] = (uint8_t)(raw & 0x00FF);
				PC_next = PC + 0x2;
				break;
			case 0x7000:	// ADD Vx, byte, 0x7xkk
				x = (raw & 0x0F00) >> 0x8;
				V[x] += (uint8_t)(raw & 0x00FF);
				PC_next = PC + 0x2;
				break;
			case 0x8000:
				x = (raw & 0x0F00) >> 0x8;
				y = (raw & 0x00F0) >> 0x4;
				switch(raw & 0x000F) {
					case 0x0000:	// LD Vx, Vy
						V[x] = V[y];
						break;
					case 0x0001:	// OR Vx, Vy
						V[x] |= V[y];
						break;
					case 0x0002:	// AND Vx, Vy
						V[x] &= V[y];
						break;
					case 0x0003:	// XOR Vx, Vy
						V[x] ^= V[y];
						break;
					case 0x0004:	// ADD Vx, Vy
						temp = V[x] + V[y];
						if(temp > 0x00FF) {
							V[0xF] = 0x01;
						} else {
							V[0xF] = 0x00;
						}
						V[x] = (temp & 0x00FF);
						break;
					case 0x0005:	// SUB Vx, Vy
						if(V[x] > V[y]) {
							V[0xF] = 0x01;
						} else {
							V[0xF] = 0x00;
						}
						V[x] -= V[y];
						break;
					case 0x0006:	// SHR Vx {, Vy}
						V[0xF] = V[x] & 0x0001;
						V[x] = V[x] >> 0x1;
						break;
					case 0x0007:	// SUBN Vx, Vy
						if(V[y] > V[x]) {
							V[0xF] = 0x01;
						} else {
							V[0xF] = 0x00;
						}
						V[x] = V[y] - V[x];
						break;
					case 0x000E:	// SHL Vx, {, Vy}
						V[0xF] = (V[x] & 0x80) >> 0x7;
						V[x] = V[x] >> 0x1;
						break;
					default:
						printf("Unknown instruction, this is a NOP.");
						PC_next = PC + 0x2;
				}
				PC_next = PC + 0x2;
				break;
			case 0x9000:	// SNE Vx, Vy
				x = (raw & 0x0F00) >> 0x8;
				y = (raw & 0x00F0) >> 0x4;
				if(V[x] != V[y]) {
					PC_next = PC + 0x4;
				} else {
					PC_next = PC + 0x2;
				}
				break;
			case 0xA000:	// LD I, addr
				I = raw & 0x0FFF;
				PC_next = PC + 0x2;
				break;
			case 0xB000:	// JP V0, addr
				PC_next = (uint16_t)(V[0]) + (raw & 0x0FFF);
				break;
			case 0xC000:	// RND Vx, byte
				x = (raw & 0x0F00) >> 0x8;
				V[x] = (uint8_t)rand() & (uint8_t)(raw & 0x00FF);
				break;
			case 0xD000:	// DRW Vx, Vy, n
				break;
			case 0xE000:
				break;
			case 0xF000:	
				x = (raw & 0x0F00) >> 0x8;
				switch(raw & 0x00FF) {
					case 0x0007:	// LD Vx, DT
						V[x] = DT;
						break;
					case 0x000A:	// LD Vx, K
						V[x] = get_hex_char();
						break;
					case 0x0015:	// LD DT, Vx
						DT = V[x];
						break;
					case 0x0018:	// LD ST, Vx
						ST = V[x];
						break;
					case 0x001E:	// ADD I, Vx
						I += (uint16_t)(V[x]);
						break;
					case 0x0029:	// LD F, Vx
						break;
					case 0x0033:	// LD B, Vx
						mem[I] = (uint8_t)((V[x] / 100) % 10);
						mem[I + 0x1] = (uint8_t)((V[x] / 10) % 10);
						mem[I + 0x2] = (uint8_t)(V[x] % 10);
						break;
					case 0x0055:	// LD [I], Vx
						for(int i = 0; i <= x; i++) {
							mem[I + i] = V[i];
						}
						break;
					case 0x0065:	// LD Vx, [I]
						for(int i = 0; i <= x; i++) {
							V[i] = mem[I + i];
						}
						break;
					default:
						printf("Unknown instruction, this is a NOP.\n");
				}
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

uint8_t get_hex_char() {
	char temp;
	printf("Waiting for input:\t");
	while(temp = getchar()){
		// Sanity check input ranges
		printf("Received raw: 0x%x\n", (uint8_t)(temp - 0x61));
		if(temp != 0xA) {
			return (uint8_t)(temp - 0x61);
		} else {
			printf("Waiting for input:\t");
		}
	}
}
