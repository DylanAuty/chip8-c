// main.c
// Contains the main loop for the chip-8 emulator.

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>


int read_sprites(uint8_t *mem);
int load_program(uint8_t *mem, char* file_name);
uint8_t draw_sprite(WINDOW* display_window, WINDOW* debug_window, uint8_t *mem, uint8_t y, uint8_t x, uint16_t bytes, uint16_t I);
void CLS(WINDOW* display_window);
uint8_t get_hex_char();
WINDOW* create_window(int height, int width, int start_y, int start_x, int border);
void destroy_window(WINDOW *window);

int main(int argc, char* argv[]) {

	// Define the RAM, 4095 (0xFFF) bytes long.
	uint8_t mem[0x1000] = {0x00};

	// Read in the sprites file to memory.
	if(read_sprites(mem) > 0) {
		return 1;
	}

	// Load in the program file provided, if there is one.
	if(argc > 1) {
		if(load_program(mem, argv[1]) > 0) {
			return 1;
		}
	}

	// Define the registers, 16 8 bit, 1 16 bit.
	uint8_t V[0x10] = {0x00};		// General purpose registers
	uint16_t I = 0x0000;			// A larger register

	// Define special registers
	uint16_t PC = 0x0200;			// Program counter. Starts at 0x200.
	uint8_t SP = 0x00;				// Stack pointer
	uint8_t DT = 0x00;				// Delay timer, counts down at 60Hz when nonzero. Stops at zero.
	uint8_t ST = 0x00;				// Sound timer, counts down at 60Hz when nonzero, with buzzer sounding throughout. Stops at zero.

	uint16_t stack[0x10] = {0x0000};	// The stack, up to 0xf return addresses allowed.

	// Variables for the main loop
	uint16_t raw = 0x0000;		// Stores value read in from memory
	uint16_t PC_next = PC;		// Assigned to PC at the end of the main loop.
	uint16_t temp = 0x0000;		// A 16 bit temporary variable. Used when doing 8 bit carry-based operations.
	uint16_t input = 0x10;		// A 16 bit temporary variable. Used when doing 8 bit carry-based operations.
	uint16_t x = 0x0000;		// To be used as primary register index
	uint16_t y = 0x0000;		// To be used as secondary register index, where needed.
	time_t start_time = time(0);	// These are used for making sure the timers decrement at 60Hz
	time_t time_difference = 0;
	int DT_temp = 0;
	int ST_temp = 0;

	int debug_ticker = 0;		// To print in debug window, should show cycles passing.
	// Display setup
	int max_term_y, max_term_x;
	initscr();
	cbreak();
	noecho();
	getmaxyx(stdscr, max_term_y, max_term_x);
	refresh();
	WINDOW *display_window = create_window(34, 66, 0, 0, 1);
	WINDOW *debug_window = create_window(max_term_y - 3, max_term_x - 66, 0, 66, 0);
	scrollok(debug_window, TRUE);
	wprintw(debug_window, "Starting. stdscr size is (%d, %d).\n", max_term_y, max_term_x);
	wrefresh(debug_window);
	getch();
	curs_set(0);

	nodelay(stdscr, TRUE);
	while(1) {
		//usleep(4000);

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
		raw = (uint16_t)(mem[PC] << 0x8) + (uint16_t)(mem[PC + 0x1]);
		/*
		wprintw(debug_window, "Reading from address 0x%x\n", PC);
		wprintw(debug_window, "=== c: %d\n", debug_ticker);
		wprintw(debug_window, "raw = 0x%04x\n", raw);
		wprintw(debug_window, "DT = 0x%04x\n", DT);
		debug_ticker++;
		
		wprintw(debug_window, "input: %x\n", input);
		wrefresh(debug_window);
		*/
		input = get_hex_char();
		switch(raw & 0xF000) {
			case 0x0000:
				switch(raw & 0x00FF) {
					case 0x00E0:	// CLS
						PC_next = PC + 0x2;
						break;
					case 0x00EE:	// RET, 0x00EE
						SP--;
						PC_next = stack[SP] + 0x02;
						break;
					default:
						wprintw(debug_window, "Unknown instruction, this is a NOP.");
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
				if(V[x] != (raw & 0x00FF)) {
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
						V[x] = V[x] << 0x1;
						break;
					default:
						wprintw(debug_window, "Unknown instruction, this is a NOP.");
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
				x = (raw & 0x0F00) >> 0x8;
				y = (raw & 0x00F0) >> 0x4;
				temp = raw & 0x000F;
				V[0xF] = draw_sprite(display_window, debug_window, mem, V[y], V[x], temp, I);
				PC_next = PC + 0x2;
				break;
			case 0xE000:
				wprintw(debug_window, "Command not implemented. This is a NOP.\n");
				PC_next = PC + 0x2;
				break;
			case 0xF000:	
				x = (raw & 0x0F00) >> 0x8;
				switch(raw & 0x00FF) {
					case 0x0007:	// LD Vx, DT
						V[x] = DT;
						break;
					case 0x000A:	// LD Vx, K
						nodelay(stdscr, FALSE);
						while(temp = get_hex_char()) {
							if(temp != 0x10) {
								V[x] = temp;
								break;
							}
						}
						nodelay(stdscr, TRUE);
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
						wprintw(debug_window, "Unknown instruction, this is a NOP.\n");
				}
				PC_next = PC + 0x2;
				break;
			default:
				wprintw(debug_window, "Default\n");
				PC_next = PC + 0x2;
				break;
		}

		// Move on
		PC = PC_next;
		//getchar();
	}
	
	destroy_window(display_window);
	destroy_window(debug_window);
	endwin();
	return 0;
}

uint8_t draw_sprite(WINDOW* display_window, WINDOW* debug_window, uint8_t *mem, uint8_t y, uint8_t x, uint16_t bytes, uint16_t I) {
	char lookup_table[0x10][0x4] = {
		{"    "},
		{"   #"},
		{"  # "},
		{"  ##"},
		{" #  "},
		{" # #"},
		{" ## "},
		{" ###"},
		{"#   "},
		{"#  #"},
		{"# # "},
		{"# ##"},
		{"##  "},
		{"## #"},
		{"### "},
		{"####"},
	};
	
	uint8_t curr_byte = 0x00;
	uint8_t existing_char;
	// Whatever the desired coordinates are, they're modded with the display dimensions to make
	// the display into a torus, and then 1 is added to allow for the border of the window.
	uint8_t collision = 0x00;		// Will be set to 0x01 when a collision happens.
	uint8_t erasure = 0x00;	// Denotes an erasure happened.
	uint8_t flip = 0x00;	// Denotes a pixel flip should happen.
	uint8_t curr_y = 0x00;
	uint8_t curr_x = 0x00;
	for(int i = 0x0; i < bytes; i++) {
		// Read what was there before, compute bitwise xor.
		// If result is greater than 0, set VF (return 1). Else return 0.
		// Draw the resulting thing onto the screen, character by character.
		curr_byte = mem[I + i];
		curr_y = ((y + i) % 0x20) + 1;
		for(int j = 0x0; j < 0x8; j++) {
			curr_x = ((x + j) % 0x40) + 1;
			existing_char = mvwinch(display_window, curr_y, curr_x);
			if(j < 0x4) {
				flip = (existing_char != ' ') ^ (lookup_table[(curr_byte & 0xF0) >> 0x4][j] != ' ');
			} else {
				flip = (existing_char != ' ') ^ (lookup_table[(curr_byte & 0x0F)][j % 0x4] != ' ');
			}
			collision += (existing_char != ' ') & (existing_char != ' ');
			//mvwaddch(display_window, curr_y, curr_x, '0');
			if(flip > 0) {
				mvwaddch(display_window, curr_y, curr_x, '#');
			} else {
				mvwaddch(display_window, curr_y, curr_x, ' ');
			}
		}
	}
	wrefresh(display_window);
	return (collision > 0);
}

void CLS(WINDOW* display_window){
	// Utility function to clear the display, redraw the border, and refresh.
	wclear(display_window);
	box(display_window, 0, 0);
	wrefresh(display_window);
	return;
}

uint8_t get_hex_char() {
	// Sanity check input ranges
	char temp;
	temp = getch();
	switch(temp) {
		case '1':
			return 0x1;
		case '2':
			return 0x2;
		case '3':
			return 0x3;
		case 'q':
			return 0x4;
		case 'w':
			return 0x5;
		case 'e':
			return 0x6;
		case 'a':
			return 0x7;
		case 's':
			return 0x8;
		case 'd':
			return 0x9;
		case 'x':
			return 0x0;
		case 'z':
			return 0xa;
		case 'c':
			return 0xb;
		case '4':
			return 0xc;
		case 'r':
			return 0xd;
		case 'f':
			return 0xe;
		case 'v':
			return 0xf;
		default:
			return 0x10;	// If this is returned it denotes an invalid key being pressed.
	}
}

WINDOW* create_window(int height, int width, int start_y, int start_x, int border) {
	// Creates a new window of the specified dimensions and surrounds it in a box.
	WINDOW *new_window;
	new_window = newwin(height, width, start_y, start_x);
	if(border == 1) {
		box(new_window, 0, 0);
	}
	wrefresh(new_window);
	return new_window;
}

void destroy_window(WINDOW *window) {
	// Destroys a window passed to it.
	wborder(window, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
	wrefresh(window);
	delwin(window);
	return;
}

int read_sprites(uint8_t *mem) {
	// Function to read the file './sprites.ch8' into memory, starting at 0x000.
	FILE *fp;
	fp = fopen("./sprites.ch8", "rb");
	if(fp == NULL) {
		fprintf(stderr, "Error: couldn't open sprites.ch8. Exiting.\n");
		return 1;
	}
	int c;
	int max = 0x200;
	for(int i = 0; i < max && (c = getc(fp)) != EOF; i++) {
		mem[i] = c;
	}
	fclose(fp);
	return 0;
}

int load_program(uint8_t *mem, char *file_name) {
	FILE *fp;
	fp = fopen(file_name, "rb");
	if(fp == NULL) {
		fprintf(stderr, "Error: couldn't open file: %s. Exiting.\n", file_name);
		return 1;
	}
	int c;
	int max = 0xFFF;
	for(int i = 0x200; i < max && (c = getc(fp)) != EOF; i++) {
		mem[i] = c;
	}
	fclose(fp);
	return 0;
	
}
