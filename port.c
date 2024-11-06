#include "port.h"
#include "chip8.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include <stdio.h>

static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Texture* texture;

/**
Keypad       Keyboard
+-+-+-+-+    +-+-+-+-+
|1|2|3|C|    |1|2|3|4|
+-+-+-+-+    +-+-+-+-+
|4|5|6|D|    |Q|W|E|R|
+-+-+-+-+ => +-+-+-+-+
|7|8|9|E|    |A|S|D|F|
+-+-+-+-+    +-+-+-+-+
|A|0|B|F|    |Z|X|C|V|
+-+-+-+-+    +-+-+-+-+
 */
static uint8_t KEY_MAP[CHIP8_KEY_SIZE] = {
  SDLK_x,  // 0
  SDLK_1,  // 1
  SDLK_2,  // 2
  SDLK_3,  // 3
  SDLK_q,  // 4
  SDLK_w,  // 5
  SDLK_e,  // 6
  SDLK_a,  // 7
  SDLK_s,  // 8
  SDLK_d,  // 9
  SDLK_z,  // A
  SDLK_c,  // B
  SDLK_4,  // C
  SDLK_r,  // D
  SDLK_f,  // E
  SDLK_v   // F
};

int display_init(const char* title, int width, int height, int scale) {
  window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                     width * scale, height * scale, SDL_WINDOW_SHOWN);
  if(!window) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow() Error: %s", SDL_GetError());
    SDL_Quit();
    return 0;
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if(!renderer) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateRenderer() Error: %s", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
  }

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                              SDL_TEXTUREACCESS_STREAMING, width, height);
  if(!texture) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateTexture() Error: %s", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
  }

  return 1;
}

void display_handle(struct chip8_t* chip8) {
  SDL_UpdateTexture(texture, NULL, chip8->gfx, sizeof(chip8->gfx[0]));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

void display_destroy() {
  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyTexture(texture);
}

static int print_dump_on = 1;

void keyboard_handle(struct chip8_t* chip8) {
  SDL_Event event;
  while(SDL_PollEvent(&event)) {
    if(event.type == SDL_QUIT) {
      chip8->state = CHIP8_STATE_QUIT;
      return;
    } else if(event.type == SDL_KEYDOWN) {
      if(event.key.keysym.sym == SDLK_ESCAPE) {
        chip8->state = CHIP8_STATE_QUIT;
        return;
      } else if(event.key.keysym.sym == SDLK_SPACE) {
        chip8->state = chip8->state == CHIP8_STATE_PAUSED ? CHIP8_STATE_PLAYING
                                                          : CHIP8_STATE_PAUSED;
      } else if(event.key.keysym.sym == SDLK_p) {
        if(print_dump_on) {
          chip8_dump_pc(chip8);
          chip8_dump_register(chip8);
          chip8_dump_memory(chip8);
          print_dump_on = 0;
        }
      } else {
        for(int i = 0; i < CHIP8_KEY_SIZE; i++) {
          if(event.key.keysym.sym == KEY_MAP[i]) {
            chip8->keystate[i] = 1;
          }
        }
      }
    } else if(event.type == SDL_KEYUP) {
      if(event.key.keysym.sym == SDLK_p) {
        print_dump_on = 1;
      } else {
        for(int i = 0; i < CHIP8_KEY_SIZE; i++) {
          if(event.key.keysym.sym == KEY_MAP[i]) {
            chip8->keystate[i] = 0;
          }
        }
      }
    }
  }
}

static Mix_Chunk *beep_chunk;

int sound_init() {
  if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096)) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Mix_OpenAudio() Error: %s\n", Mix_GetError());
    SDL_Quit();
    return 0;
  }
  beep_chunk = Mix_LoadWAV("beep.wav");
  if(!beep_chunk) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not load beep.wav: %s\n", Mix_GetError());
    Mix_CloseAudio();
    SDL_Quit();
    return 0;
  }
  return 1;
}

void sound_handle(struct chip8_t* chip8) {
  int channel = Mix_PlayChannel(-1, beep_chunk, 0);
  if(channel == -1) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to play chirp: %s\n", Mix_GetError());
    Mix_FreeChunk(beep_chunk);
    Mix_CloseAudio();
    SDL_Quit();
  }
}

void sound_destroy() {
  Mix_FreeChunk(beep_chunk);
  Mix_Quit();
}