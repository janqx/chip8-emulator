#include "chip8.h"

#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static uint8_t CHIP8_FONTSET[CHIP8_FONTSET_SIZE] = {
  0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */
  0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
  0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */
  0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */
  0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */
  0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */
  0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */
  0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */
  0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */
  0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */
  0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */
  0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */
  0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */
  0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */
  0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */
  0xF0, 0x80, 0xF0, 0x80, 0x80  /* F */
};

#if defined(NDEBUG)
static inline void debug_printf(const char* fmt, ...) {}
#else
static inline void debug_printf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n\n");
}
#endif

static inline void opcode_raw(struct chip8_t* chip8) {
  debug_printf("raw 0x%.4X", chip8->opcode);
  chip8->pc += 2;
}

static inline void opcode_00E0(struct chip8_t* chip8) {
  debug_printf("cls");
  memset(chip8->gfx, 0, sizeof(chip8->gfx));
  chip8->draw_flag = 1;
  chip8->pc += 2;
}

static inline void opcode_00EE(struct chip8_t* chip8) {
  debug_printf("ret");
  chip8->pc = chip8->stack[--chip8->sp];
}

static inline void opcode_1NNN(struct chip8_t* chip8) {
  debug_printf("jp 0x%X", chip8->D.NNN);
  chip8->pc = chip8->D.NNN;
}

static inline void opcode_2NNN(struct chip8_t* chip8) {
  debug_printf("call 0x%X", chip8->D.NNN);
  chip8->stack[chip8->sp++] = chip8->pc + 2;
  chip8->pc = chip8->D.NNN;
}

static inline void opcode_3XNN(struct chip8_t* chip8) {
  debug_printf("se v%d, 0x%X", chip8->D.X, chip8->D.NN);
  chip8->pc += (chip8->V[chip8->D.X] == chip8->D.NN) ? 4 : 2;
}

static inline void opcode_4XNN(struct chip8_t* chip8) {
  debug_printf("sne v%d, 0x%X", chip8->D.X, chip8->D.NN);
  chip8->pc += (chip8->V[chip8->D.X] != chip8->D.NN) ? 4 : 2;
}

static inline void opcode_5XY0(struct chip8_t* chip8) {
  debug_printf("se v%d, v%d", chip8->D.X, chip8->D.Y);
  chip8->pc += (chip8->V[chip8->D.X] == chip8->V[chip8->D.Y]) ? 4 : 2;
}

static inline void opcode_6XNN(struct chip8_t* chip8) {
  debug_printf("ld v%d, 0x%X", chip8->D.X, chip8->D.NN);
  chip8->V[chip8->D.X] = chip8->D.NN;
  chip8->pc += 2;
}

static inline void opcode_7XNN(struct chip8_t* chip8) {
  debug_printf("add v%d, 0x%X", chip8->D.X, chip8->D.NN);
  chip8->V[chip8->D.X] += chip8->D.NN;
  chip8->pc += 2;
}

static inline void opcode_8XY0(struct chip8_t* chip8) {
  debug_printf("ld v%d, v%d", chip8->D.X, chip8->D.Y);
  chip8->V[chip8->D.X] = chip8->V[chip8->D.Y];
  chip8->pc += 2;
}

static inline void opcode_8XY1(struct chip8_t* chip8) {
  debug_printf("or v%d, v%d", chip8->D.X, chip8->D.Y);
  chip8->V[chip8->D.X] |= chip8->V[chip8->D.Y];
  chip8->pc += 2;
}

static inline void opcode_8XY2(struct chip8_t* chip8) {
  debug_printf("and v%d, v%d", chip8->D.X, chip8->D.Y);
  chip8->V[chip8->D.X] &= chip8->V[chip8->D.Y];
  chip8->pc += 2;
}

static inline void opcode_8XY3(struct chip8_t* chip8) {
  debug_printf("xor v%d, v%d", chip8->D.X, chip8->D.Y);
  chip8->V[chip8->D.X] ^= chip8->V[chip8->D.Y];
  chip8->pc += 2;
}

static inline void opcode_8XY4(struct chip8_t* chip8) {
  debug_printf("add v%d, v%d", chip8->D.X, chip8->D.Y);
  uint16_t added = (uint16_t)(((uint16_t)chip8->V[chip8->D.X]) +
                              ((uint16_t)chip8->V[chip8->D.Y]));
  chip8->V[chip8->D.X] = added & 0xFF;
  chip8->V[0xF] = (added & 0x0100) >> 8;
  chip8->pc += 2;
}

static inline void opcode_8XY5(struct chip8_t* chip8) {
  debug_printf("sub v%d, v%d", chip8->D.X, chip8->D.Y);
  if(chip8->V[chip8->D.X] > chip8->V[chip8->D.Y]) {
    chip8->V[0xF] = 1;
  } else {
    chip8->V[0xF] = 0;
  }
  chip8->V[chip8->D.X] -= chip8->V[chip8->D.Y];
  chip8->pc += 2;
}

static inline void opcode_8XY6(struct chip8_t* chip8) {
  debug_printf("shr v%d", chip8->D.X);
  chip8->V[0xF] = chip8->V[chip8->D.X] & 0x01;
  chip8->V[chip8->D.X] >>= 1;
  chip8->pc += 2;
}

static inline void opcode_8XY7(struct chip8_t* chip8) {
  debug_printf("subn v%d, v%d", chip8->D.X, chip8->D.Y);
  if(chip8->V[chip8->D.X] < chip8->V[chip8->D.Y]) {
    chip8->V[0xF] = 1;
  } else {
    chip8->V[0xF] = 0;
  }
  chip8->V[chip8->D.X] = chip8->V[chip8->D.Y] - chip8->V[chip8->D.X];
  chip8->pc += 2;
}

static inline void opcode_8XYE(struct chip8_t* chip8) {
  debug_printf("shl v%d", chip8->D.X);
  chip8->V[0xF] = (chip8->V[chip8->D.X] * 0x80) >> 7;
  chip8->V[chip8->D.X] <<= 1;
  chip8->pc += 2;
}

static inline void opcode_9XY0(struct chip8_t* chip8) {
  debug_printf("sne v%d, v%d", chip8->D.X, chip8->D.Y);
  chip8->pc += chip8->V[chip8->D.X] != chip8->V[chip8->D.Y] ? 4 : 2;
}

static inline void opcode_ANNN(struct chip8_t* chip8) {
  debug_printf("ld I, 0x%x", chip8->D.NNN);
  chip8->I = chip8->D.NNN;
  chip8->pc += 2;
}

static inline void opcode_BNNN(struct chip8_t* chip8) {
  debug_printf("jp v0, 0x%x", chip8->D.NNN);
  chip8->pc = chip8->V[0x0] + chip8->D.NNN;
}

static inline void opcode_CXNN(struct chip8_t* chip8) {
  debug_printf("rnd v%d, 0x%x", chip8->D.X, chip8->D.NN);
  chip8->V[chip8->D.X] = (rand() % 256) & chip8->D.NN;
  chip8->pc += 2;
}

static inline void opcode_DXYN(struct chip8_t* chip8) {
  debug_printf("drw v%d, v%d, 0x%x", chip8->D.X, chip8->D.Y, chip8->D.N);
  chip8->V[0xF] = 0;
  uint8_t sx = chip8->V[chip8->D.X];
  uint8_t sy = chip8->V[chip8->D.Y];
  uint8_t height = chip8->D.N;
  for(uint8_t i = 0; i < height; i++) {
    for(uint8_t j = 0; j < 8; j++) {
      uint8_t pixel = (chip8->memory[chip8->I + (uint16_t)i] & (0x80 >> j));
      uint8_t cx = sx + j;
      uint8_t cy = sy + i;
      if(cx >= CHIP8_DISPLAY_WIDTH || cy >= CHIP8_DISPLAY_HEIGHT) {
        continue;
      }
      if(pixel) {
        if(chip8->gfx[cy][cx] == CHIP8_DISPLAY_WHITE) {
          chip8->V[0xF] = 1;
        }
        chip8->gfx[cy][cx] = chip8->gfx[cy][cx] == CHIP8_DISPLAY_WHITE
                               ? CHIP8_DISPLAY_BLACK
                               : CHIP8_DISPLAY_WHITE;
      }
    }
  }
  chip8->draw_flag = 1;
  chip8->pc += 2;
}
static inline void opcode_EX9E(struct chip8_t* chip8) {
  debug_printf("skp v%d", chip8->D.X);
  chip8->pc += chip8->keystate[chip8->V[chip8->D.X]] ? 4 : 2;
}

static inline void opcode_EXA1(struct chip8_t* chip8) {
  debug_printf("sknp v%d", chip8->D.X);
  chip8->pc += !chip8->keystate[chip8->V[chip8->D.X]] ? 4 : 2;
}

static inline void opcode_FX07(struct chip8_t* chip8) {
  debug_printf("ld v%d, DT", chip8->D.X);
  chip8->V[chip8->D.X] = chip8->delay_timer;
  chip8->pc += 2;
}

static inline void opcode_FX0A(struct chip8_t* chip8) {
  debug_printf("ld v%d, K", chip8->D.X);
  int keypressed = 0;
  for(uint8_t i = 0; i < CHIP8_KEY_SIZE; i++) {
    if(chip8->keystate[i]) {
      chip8->V[chip8->D.X] = i;
      keypressed = 1;
    }
  }
  if(keypressed) {
    chip8->pc += 2;
  }
}

static inline void opcode_FX15(struct chip8_t* chip8) {
  debug_printf("ld DT, v%d", chip8->D.X);
  chip8->delay_timer = chip8->V[chip8->D.X];
  chip8->pc += 2;
}

static inline void opcode_FX18(struct chip8_t* chip8) {
  debug_printf("ld ST, v%d", chip8->D.X);
  chip8->sound_timer = chip8->V[chip8->D.X];
  chip8->pc += 2;
}

static inline void opcode_FX1E(struct chip8_t* chip8) {
  debug_printf("add I, v%d", chip8->D.X);
  chip8->I += chip8->V[chip8->D.X];
  chip8->pc += 2;
}

static inline void opcode_FX29(struct chip8_t* chip8) {
  debug_printf("ld F, v%d", chip8->D.X);
  chip8->I = CHIP8_FONTSET_MEM_START + chip8->V[chip8->D.X] * 5;
  chip8->pc += 2;
}

static inline void opcode_FX33(struct chip8_t* chip8) {
  debug_printf("ld B, v%d", chip8->D.X);
  uint8_t x = chip8->V[chip8->D.X];
  chip8->memory[chip8->I] = (x % 1000) / 100;
  chip8->memory[chip8->I + 1] = (x % 100) / 10;
  chip8->memory[chip8->I + 2] = (x % 10);
  chip8->pc += 2;
}

static inline void opcode_FX55(struct chip8_t* chip8) {
  debug_printf("ld [I], v%d", chip8->D.X);
  for(uint8_t i = 0; i <= chip8->D.X; i++) {
    chip8->memory[chip8->I + i] = chip8->V[i];
  }
  chip8->pc += 2;
}

static inline void opcode_FX65(struct chip8_t* chip8) {
  debug_printf("ld v%d, [I]", chip8->D.X);
  for(uint8_t i = 0; i <= chip8->D.X; i++) {
    chip8->V[i] = chip8->memory[chip8->I + i];
  }
  chip8->pc += 2;
}

void chip8_init(struct chip8_t* chip8) {
  memset(chip8, 0, sizeof(struct chip8_t));
  for(int i = 0; i < CHIP8_FONTSET_SIZE; i++) {
    chip8->memory[CHIP8_FONTSET_MEM_START + i] = CHIP8_FONTSET[i];
  }
  chip8->state = CHIP8_STATE_READY;
}

int chip8_load_program(struct chip8_t* chip8, const char* filename) {
  FILE* fp = fopen(filename, "rb");
  if(!fp) {
    fprintf(stderr, "can't open file: '%s'", filename);
    return 0;
  }
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  if(size > CHIP8_MEMORY_SIZE - CHIP8_MEMORY_START) {
    fprintf(stderr, "the rom file is too large");
    fclose(fp);
    return 0;
  }
  fseek(fp, 0, SEEK_SET);
  fread(chip8->memory + CHIP8_MEMORY_START, 1, size, fp);
  fclose(fp);
  chip8->pc = CHIP8_MEMORY_START;
  chip8->state = CHIP8_STATE_PLAYING;
  return 1;
}

void chip8_cricle(struct chip8_t* chip8) {
  // fetch
  chip8->opcode = ((chip8->memory[chip8->pc] << 8) & 0xff00) |
                  (chip8->memory[chip8->pc + 1] & 0xff);

  // decode
  chip8->D.I = ((chip8->opcode & 0xF000u) >> 12);
  chip8->D.X = ((chip8->opcode & 0x0F00u) >> 8);
  chip8->D.Y = ((chip8->opcode & 0x00F0u) >> 4);
  chip8->D.N = (chip8->opcode & 0x000Fu);
  chip8->D.NN = (chip8->opcode & 0x00FFu);
  chip8->D.NNN = (chip8->opcode & 0x0FFFu);

  // execute
  switch(chip8->D.I) {
    case 0x0:
      switch(chip8->D.NN) {
        case 0xE0:
          opcode_00E0(chip8);
          return;
        case 0xEE:
          opcode_00EE(chip8);
          return;
      }
      break;
    case 0x1:
      opcode_1NNN(chip8);
      return;
    case 0x2:
      opcode_2NNN(chip8);
      return;
    case 0x3:
      opcode_3XNN(chip8);
      return;
    case 0x4:
      opcode_4XNN(chip8);
      return;
    case 0x5:
      opcode_5XY0(chip8);
      return;
    case 0x6:
      opcode_6XNN(chip8);
      return;
    case 0x7:
      opcode_7XNN(chip8);
      return;
    case 0x8:
      switch(chip8->D.N) {
        case 0x0:
          opcode_8XY0(chip8);
          return;
        case 0x1:
          opcode_8XY1(chip8);
          return;
        case 0x2:
          opcode_8XY2(chip8);
          return;
        case 0x3:
          opcode_8XY3(chip8);
          return;
        case 0x4:
          opcode_8XY4(chip8);
          return;
        case 0x5:
          opcode_8XY5(chip8);
          return;
        case 0x6:
          opcode_8XY6(chip8);
          return;
        case 0x7:
          opcode_8XY7(chip8);
          return;
        case 0xe:
          opcode_8XYE(chip8);
          return;
      }
      break;
    case 0x9:
      opcode_9XY0(chip8);
      return;
    case 0xA:
      opcode_ANNN(chip8);
      return;
    case 0xB:
      opcode_BNNN(chip8);
      return;
    case 0xC:
      opcode_CXNN(chip8);
      return;
    case 0xD:
      opcode_DXYN(chip8);
      return;
    case 0xE:
      switch(chip8->D.NN) {
        case 0x9E:
          opcode_EX9E(chip8);
          return;
        case 0xA1:
          opcode_EXA1(chip8);
          return;
      }
      break;
    case 0xF:
      switch(chip8->D.NN) {
        case 0x07:
          opcode_FX07(chip8);
          return;
        case 0x0A:
          opcode_FX0A(chip8);
          return;
        case 0x15:
          opcode_FX15(chip8);
          return;
        case 0x18:
          opcode_FX18(chip8);
          return;
        case 0x1E:
          opcode_FX1E(chip8);
          return;
        case 0x29:
          opcode_FX29(chip8);
          return;
        case 0x33:
          opcode_FX33(chip8);
          return;
        case 0x55:
          opcode_FX55(chip8);
          return;
        case 0x65:
          opcode_FX65(chip8);
          return;
      }
      break;
  }
  opcode_raw(chip8);
}

void chip8_dump_pc(struct chip8_t* chip8) {
  printf("\n\nDump Program Counter:\nPC: 0x%.4X\nOpcode: 0x%.4X\n", chip8->pc,
         chip8->opcode);
}

void chip8_dump_register(struct chip8_t* chip8) {
  printf("\n\nDump Registers:\n");
  for(int i = 0; i < CHIP8_REGISTER_SIZE; i += 4) {
    printf("V%X=0x%X\tV%X=0x%X\tV%X=0x%X\tV%X=0x%X\n", i, chip8->V[i], i + 1,
           chip8->V[i + 1], i + 2, chip8->V[i + 2], i + 3, chip8->V[i + 3]);
  }
}

void chip8_dump_memory(struct chip8_t* chip8) {
  printf("\n\nDump Memory(hex):\n");
  for(int i = 0; i < CHIP8_MEMORY_SIZE; i += 8) {
    printf("%.4X:  %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n", i,
           chip8->memory[i], chip8->memory[i + 1], chip8->memory[i + 2],
           chip8->memory[i + 3], chip8->memory[i + 4], chip8->memory[i + 5],
           chip8->memory[i + 6], chip8->memory[i + 7]);
  }
}
