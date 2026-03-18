#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

static bool running;

static SDL_Window *window;
static SDL_Renderer *renderer;

static SDL_Texture *back_buffer;

static uint32_t *pixels;
static int bitmap_width, bitmap_height;

static int offset;

static void render_back_buffer(int x_offset, int y_offset) {
    SDL_GetWindowSize(window, &bitmap_width, &bitmap_height);

    pixels = realloc(pixels, bitmap_height*bitmap_width * sizeof(uint32_t));
    uint32_t *pixel = pixels;
    for (int y = 0; y < bitmap_height; ++y) {
        for (int x = 0; x < bitmap_width; ++x) {
            uint8_t red = x + x_offset;
            uint8_t green = y + y_offset;
            uint8_t blue = 0;
            *pixel++ = (blue << 16) | (green << 8) | red;
        }
    }

    SDL_UpdateTexture(back_buffer, &(SDL_Rect){0, 0, bitmap_width, bitmap_height}, pixels, bitmap_width * 4);
}

static void process_event(SDL_Event event) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
        running = false;
        break;
    case SDL_EVENT_KEY_DOWN:
        switch (event.key.scancode) {
        case SDL_SCANCODE_ESCAPE:
            running = false;
            break;
        }
    case SDL_EVENT_WINDOW_RESIZED:
        render_back_buffer(offset, offset);
    }
}

int main(int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        printf("Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (!SDL_CreateWindowAndRenderer("Handmade Hero", 0, 0, SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS, &window, &renderer)) {
        printf("Couldn't create window/renderer: %s", SDL_GetError());
        return 1;
    }

    SDL_GetWindowSize(window, &bitmap_width, &bitmap_height);
    if (!(back_buffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBX32, SDL_TEXTUREACCESS_STREAMING, bitmap_width, bitmap_height))) {
        printf("Couldn't create back_buffer texture: %s", SDL_GetError());
        return 1;
    }

    pixels = malloc(sizeof(uint32_t) * bitmap_width*bitmap_height);

    running = true;
    while (running) {
        SDL_RenderClear(renderer);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            process_event(event);
        }

        render_back_buffer(offset, offset);

        SDL_RenderTexture(renderer, back_buffer, &(SDL_FRect){0, 0, bitmap_width, bitmap_height}, &(SDL_FRect){0, 0, bitmap_width, bitmap_height});
        SDL_RenderPresent(renderer);

        offset++;
    }

    SDL_DestroyTexture(back_buffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}
