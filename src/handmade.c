#include "headers/handmade.h"

void render_back_buffer(GameBackBuffer *back_buffer, uint8_t x_offset, uint8_t y_offset) {
    uint8_t *row = (uint8_t*)back_buffer->memory;
    for (int y = 0; y < back_buffer->height; ++y) {
        uint32_t *pixel = (uint32_t*)row;
        for (int x = 0; x < back_buffer->width; ++x) {
            uint8_t red = (uint8_t)x + x_offset;
            uint8_t green = (uint8_t)y + y_offset;
            uint8_t blue = 0;

            *pixel++ = (uint32_t)(blue << 16) | (uint32_t)(green << 8) | (uint32_t)red;
        }
        row += back_buffer->pitch;
    }
}

void game_update_and_render(GameBackBuffer *back_buffer, uint8_t x_offset, uint8_t y_offset) {
    render_back_buffer(back_buffer, x_offset, y_offset);
}
