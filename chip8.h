#pragma once

#include <stdint.h>

#define CHIP8_DISPLAY_HEIGHT 32
#define CHIP8_DISPLAY_WIDTH 64
#define CHIP8_DISPLAY_SCALE 15
#define CHIP8_DISPLAY_WHITE 0xFFFFFFFF
#define CHIP8_DISPLAY_BLACK 0x00000000

#define CHIP8_MEMORY_SIZE 4096
#define CHIP8_MEMORY_START 0x200
#define CHIP8_STACK_SIZE 16
#define CHIP8_KEY_SIZE 16
#define CHIP8_REGISTER_SIZE 16

#define CHIP8_FONTSET_SIZE 80
#define CHIP8_FONTSET_MEM_START 0x50

#define CYCLE_DELAY (1000 / 1000)
#define TIMER_DELAY (1000 / 60)

#define CHIP8_STATE_READY 0
#define CHIP8_STATE_QUIT 1
#define CHIP8_STATE_PLAYING 2
#define CHIP8_STATE_PAUSED 3

struct chip8_t {
  int state;
  uint8_t V[CHIP8_REGISTER_SIZE];
  uint16_t I;
  uint16_t pc;
  uint16_t opcode;
  struct {    
    uint8_t I;
    uint8_t X;
    uint8_t Y;
    uint8_t N;
    uint8_t NN;
    uint16_t NNN;
    uint8_t KK;
  } D;
  uint8_t delay_timer, sound_timer;
  uint8_t memory[CHIP8_MEMORY_SIZE];
  uint16_t stack[CHIP8_STACK_SIZE];
  uint8_t sp;
  uint16_t keystate[CHIP8_KEY_SIZE];
  uint32_t gfx[CHIP8_DISPLAY_HEIGHT][CHIP8_DISPLAY_WIDTH];
  uint8_t draw_flag;
};

void chip8_init(struct chip8_t* chip8);

int chip8_load_program(struct chip8_t* chip8, const char* filename);

void chip8_cricle(struct chip8_t* chip8);

void chip8_dump_pc(struct chip8_t* chip8);

void chip8_dump_register(struct chip8_t* chip8);

void chip8_dump_memory(struct chip8_t* chip8);