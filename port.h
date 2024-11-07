#pragma once

struct chip8_t;

int display_init(const char* title, int width, int height, int scale);

void display_handle(struct chip8_t* chip8);

void display_destroy();

void keyboard_handle(struct chip8_t* chip8);

int sound_init();

void sound_handle(struct chip8_t* chip8);

void sound_destroy();

void timer_handle(struct chip8_t* chip8);