#ifndef PLATFORM_H
#define PLATFORM_H

#include <SDL3/SDL.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>

#define PI 3.14159265359f

#define FPS 60
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define BYTES_PER_PIXEL 4

#define AUDIO_SAMPLE_RATE 48000
#define BYTES_PER_SAMPLE 4

#define GAMEPAD_STICK_THRESHOLD 8000

typedef struct {
    SDL_Texture *texture;
    uint32_t *memory;
    uint16_t width;
    uint16_t height;
    int pitch;
} BackBuffer;

typedef struct {
    void *memory;
    size_t len;
    int32_t play_cursor;
    int16_t volume;
    size_t samples;
    size_t latency_bytes;
    int tone_hz;
    float period;
    float sine_t;
} AudioBuffer;

typedef struct {
    bool is_down;
    bool was_down;
} Button;

void resize_back_buffer(BackBuffer *back_buffer, uint8_t x_offset, uint8_t y_offset);
void fill_audio_buffer(AudioBuffer *audio_buffer);
void process_gamepad_button(SDL_GamepadButtonEvent gbutton);
void process_key(SDL_KeyboardEvent key);
void process_event(SDL_Event event, BackBuffer *back_buffer, uint8_t x_offset, uint8_t y_offset);
int game_init(BackBuffer *back_buffer, AudioBuffer *audio_buffer);
void game_tick(uint8_t *x_offset, uint8_t *y_offset);

#endif
