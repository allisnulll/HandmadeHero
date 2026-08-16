#ifndef HANDMADE_H
#define HANDMADE_H

#include <stdint.h>

typedef struct {
    uint32_t *memory;
    uint16_t width;
    uint16_t height;
    int pitch;
} GameBackBuffer;

void render_back_buffer(GameBackBuffer *back_buffer, uint8_t x_offset, uint8_t y_offset);
void game_update_and_render(GameBackBuffer *back_buffer, uint8_t x_offset, uint8_t y_offset);

#endif
