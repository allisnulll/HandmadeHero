#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

static bool running;

static SDL_Window *window;
static SDL_Renderer *renderer;

typedef struct {
    SDL_Texture *texture;
    uint32_t *memory;
    int width;
    int height;
    int pitch;
    int bytes_per_pixel;
} BackBuffer;

static void render_back_buffer(BackBuffer *back_buffer, int red_offset, int green_offset) {
    uint8_t *row = (uint8_t *)back_buffer->memory;
    for (int y = 0; y < back_buffer->height; ++y) {
        uint32_t *pixel = (uint32_t *)row;
        for (int x = 0; x < back_buffer->width; ++x) {
            uint8_t red = x + red_offset;
            uint8_t green = y + green_offset;
            uint8_t blue = 0;

            *pixel++ = (blue << 16) | (green << 8) | red;
        }
        row += back_buffer->pitch;
    }

    SDL_UpdateTexture(back_buffer->texture, &(SDL_Rect){0, 0, back_buffer->width, back_buffer->height}, back_buffer->memory, back_buffer->pitch);
}

static void resize_back_buffer(BackBuffer *back_buffer, int red_offset, int green_offset) {
    SDL_GetWindowSize(window, &back_buffer->width, &back_buffer->height);
    back_buffer->pitch = back_buffer->width * back_buffer->bytes_per_pixel;

    uint32_t *new_memory = realloc(back_buffer->memory, back_buffer->height*back_buffer->width * sizeof(uint32_t));
    if (!new_memory) {
        printf("realloc failed, frame skipped.\n");
        return;
    }
    back_buffer->memory = new_memory;

    render_back_buffer(back_buffer, red_offset, green_offset);
}

static void process_event(SDL_Event event, BackBuffer *back_buffer, int offset) {
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
        resize_back_buffer(back_buffer, offset, offset);
        resize_back_buffer(back_buffer, offset, offset);
    }
}

int main(int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (!SDL_CreateWindowAndRenderer("Handmade Hero", 1280, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS, &window, &renderer)) {
        printf("Couldn't create window/renderer: %s\n", SDL_GetError());
        return 1;
    }

    BackBuffer back_buffer = {.bytes_per_pixel = 4};
    SDL_GetWindowSize(window, &back_buffer.width, &back_buffer.height);
    back_buffer.pitch = back_buffer.width * back_buffer.bytes_per_pixel;
    if (!(back_buffer.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBX32, SDL_TEXTUREACCESS_STREAMING, back_buffer.width, back_buffer.height))) {
        printf("Couldn't create back_buffer texture: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        return 1;
    }

    back_buffer.memory = malloc(sizeof(uint32_t) * back_buffer.width*back_buffer.height);
    if (!back_buffer.memory) {
        printf("Couldn't allocate memory for back buffer.\n");
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyTexture(back_buffer.texture);
        return 1;
    }

    int offset = 0;
    running = true;
    while (running) {
        SDL_RenderClear(renderer);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            process_event(event, &back_buffer, offset);
        }

        render_back_buffer(&back_buffer, offset, offset);

        SDL_FRect frect = {0, 0, back_buffer.width, back_buffer.height};
        SDL_RenderTexture(renderer, back_buffer.texture, &frect, &frect);
        SDL_RenderPresent(renderer);

        offset++;
    }

    free(back_buffer.memory);

    SDL_DestroyTexture(back_buffer.texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}
