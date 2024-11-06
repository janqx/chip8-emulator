#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>

#include "chip8.h"
#include "port.h"

#include <stdio.h>

int main(int argc, char const *argv[]) {
  if(argc < 2) {
    printf("Usage: chip8-emulator <rom file>");
    return EXIT_FAILURE;
  }

  if(SDL_Init(SDL_INIT_VIDEO)) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init() Error: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  if(!display_init("Chip-8 Emulator", CHIP8_DISPLAY_WIDTH, CHIP8_DISPLAY_HEIGHT, CHIP8_DISPLAY_SCALE)) {
    return EXIT_FAILURE;
  }

  if(!sound_init()) {
    return EXIT_FAILURE;
  }

  struct chip8_t chip8;

  chip8_init(&chip8);
  if(!chip8_load_program(&chip8, argv[1])) {
    return EXIT_FAILURE;
  }

  Uint32 last_cycle_time = SDL_GetTicks();
  Uint32 last_timer_time = SDL_GetTicks();

  while(chip8.state != CHIP8_STATE_QUIT) {
    do {
      keyboard_handle(&chip8);
    } while(chip8.state == CHIP8_STATE_PAUSED);
    if(SDL_GetTicks() - last_cycle_time >= CYCLE_DELAY) {
      last_cycle_time = SDL_GetTicks();
      chip8_cricle(&chip8);
      if(SDL_GetTicks() - last_timer_time >= TIMER_DELAY) {
        last_timer_time = SDL_GetTicks();
        if(chip8.delay_timer > 0) {
          chip8.delay_timer--;
        }
        if(chip8.sound_timer > 0) {
          if(chip8.sound_timer == 1) {
            sound_handle(&chip8);
          }
          chip8.sound_timer--;
        }
      }      
    }
    if(chip8.draw_flag) {
      display_handle(&chip8);
      chip8.draw_flag = 0;
    }
  }

  display_destroy();
  sound_destroy();
  SDL_Quit();

  return EXIT_SUCCESS;
}
