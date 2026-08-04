#include <SDL3/SDL.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define BYTES_PER_PIXEL 4
#define AUDIO_SAMPLE_RATE 48000
#define GAMEPAD_STICK_THRESHOLD 8000

#define PI 3.14169f
bool is_running;

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Gamepad *gamepad;
SDL_AudioStream *audio_stream;

typedef struct {
    SDL_Texture *texture;
    uint32_t *memory;
    uint16_t width;
    uint16_t height;
    int pitch;
} BackBuffer;

typedef struct {
    void *memory;
    int32_t running_sample;
    int16_t volume;
    size_t len;
    size_t samples;
    int hz;
    float period;
} AudioBuffer;

uint8_t x_offset, y_offset;
int gaxis_left_x, gaxis_left_y;

typedef struct {
    bool is_down;
    bool was_down;
} Button;
Button up, down, left, right;

void render_back_buffer(BackBuffer *back_buffer);
void resize_back_buffer(BackBuffer *back_buffer);
void fill_audio_buffer(AudioBuffer *audio_buffer);
void process_gamepad_button(SDL_GamepadButtonEvent gbutton);
void process_key(SDL_KeyboardEvent key);
void process_event(SDL_Event event, BackBuffer *back_buffer);
int game_init(BackBuffer *back_buffer, AudioBuffer *audio_buffer);
void game_tick(void);

void render_back_buffer(BackBuffer *back_buffer) {
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

    SDL_UpdateTexture(
        back_buffer->texture,
        &(SDL_Rect){0, 0, back_buffer->width, back_buffer->height},
        back_buffer->memory,
        back_buffer->pitch
    );
}

void resize_back_buffer(BackBuffer *back_buffer) {
    SDL_GetWindowSize(window, (int*)&back_buffer->width, (int*)&back_buffer->height);
    back_buffer->pitch = back_buffer->width * BYTES_PER_PIXEL;

    uint32_t *new_memory = realloc(
        back_buffer->memory,
        back_buffer->height*back_buffer->width * sizeof(uint32_t)
    );
    if (!new_memory) {
        printf("realloc failed, frame skipped.\n");
        return;
    }
    back_buffer->memory = new_memory;

    render_back_buffer(back_buffer);
}

void fill_audio_buffer(AudioBuffer *audio_buffer) {
    if (!audio_stream) return;

    audio_buffer->len = (AUDIO_SAMPLE_RATE * sizeof(int16_t)*2);
    int queued = SDL_GetAudioStreamQueued(audio_stream);
    if (queued > 0) {
        audio_buffer->len -= (uint32_t)queued;
        if (audio_buffer->len <= 0) return;
    }
    audio_buffer->samples = audio_buffer->len / sizeof(int16_t) / 2;
    if (audio_buffer->running_sample >= AUDIO_SAMPLE_RATE)
        audio_buffer->running_sample = 0;

    int32_t starting_sample = audio_buffer->running_sample;
    size_t len1 = 0;
    for (size_t i = 0; i < audio_buffer->samples; ++i) {
        if (audio_buffer->running_sample >= AUDIO_SAMPLE_RATE) {
            audio_buffer->running_sample = 0;
            len1 = i * sizeof(int16_t) * 2;
        }

        int16_t sound = (int16_t)(sin(2.0f*PI * (float)audio_buffer->running_sample / audio_buffer->period) * (float)audio_buffer->volume);

        ((int16_t*)audio_buffer->memory)[audio_buffer->running_sample*2] = sound;
        ((int16_t*)audio_buffer->memory)[audio_buffer->running_sample*2+1] = sound;

        ++audio_buffer->running_sample;
    }

    if (len1) {
        printf("HELLO");
        SDL_PutAudioStreamData(audio_stream, (int32_t*)audio_buffer->memory + starting_sample, (int)len1);
        SDL_PutAudioStreamData(audio_stream, (int32_t*)audio_buffer->memory, (int)(audio_buffer->len - len1));
    } else
        SDL_PutAudioStreamData(audio_stream, (int32_t*)audio_buffer->memory + starting_sample, (int)audio_buffer->len);
}

void process_gamepad_button(SDL_GamepadButtonEvent gbutton) {
    if (gbutton.which != SDL_GetGamepadID(gamepad)) return;

    switch(gbutton.button) {
    case SDL_GAMEPAD_BUTTON_EAST:
        is_running = false;
        break;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        right.was_down = right.is_down;
        right.is_down = (gbutton.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
        break;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        left.was_down = left.is_down;
        left.is_down = (gbutton.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
        break;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        down.was_down = down.is_down;
        down.is_down = (gbutton.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
        break;
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        up.was_down = up.is_down;
        up.is_down = (gbutton.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
    }
}

void process_key(SDL_KeyboardEvent key) {
    switch (key.key) {
    case SDLK_ESCAPE:
    case SDLK_Q:
        is_running = false;
        break;
    case SDLK_F11:
    case SDLK_F:
        if (key.type == SDL_EVENT_KEY_DOWN)
            SDL_SetWindowFullscreen(
                window,
                !((SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) == true)
            );
        break;

    case SDLK_D:
    case SDLK_RIGHT:
        right.was_down = right.is_down;
        right.is_down = (key.type == SDL_EVENT_KEY_DOWN);
        break;
    case SDLK_A:
    case SDLK_LEFT:
        left.was_down = left.is_down;
        left.is_down = (key.type == SDL_EVENT_KEY_DOWN);
        break;
    case SDLK_S:
    case SDLK_DOWN:
        down.was_down = down.is_down;
        down.is_down = (key.type == SDL_EVENT_KEY_DOWN);
        break;
    case SDLK_W:
    case SDLK_UP:
        up.was_down = up.is_down;
        up.is_down = (key.type == SDL_EVENT_KEY_DOWN);
    }
}

void process_event(SDL_Event event, BackBuffer *back_buffer) {
    switch (event.type) {

    case SDL_EVENT_QUIT:
        is_running = false;
        break;
    case SDL_EVENT_WINDOW_RESIZED:
        resize_back_buffer(back_buffer);
        break;

    case SDL_EVENT_AUDIO_DEVICE_ADDED: {
        SDL_AudioStream *temp = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
            &(SDL_AudioSpec){SDL_AUDIO_S16LE, 2, AUDIO_SAMPLE_RATE},
            NULL,
            NULL
        );
        if (!temp) {
            printf("Failed to open audio stream: %s\n", SDL_GetError());
            break;
        }

        if (audio_stream) SDL_DestroyAudioStream(audio_stream);
        audio_stream = temp;

        SDL_ResumeAudioStreamDevice(audio_stream);

        break; }
    case SDL_EVENT_AUDIO_DEVICE_REMOVED:
        if (audio_stream && SDL_GetAudioStreamDevice(audio_stream) == event.adevice.which) {
            SDL_DestroyAudioStream(audio_stream);
            audio_stream = NULL;
        }
        break;

    case SDL_EVENT_GAMEPAD_ADDED:
        if (!gamepad) {
            gamepad = SDL_OpenGamepad(event.gdevice.which);
            if (!gamepad) {
                printf("Failed to open gamepad ID %u: %s\n", (unsigned int)event.gdevice.which, SDL_GetError());
                break;
            }
        }
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        if (gamepad && SDL_GetGamepadID(gamepad) == event.gdevice.which) {
            SDL_CloseGamepad(gamepad);
            gamepad = NULL;
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        process_gamepad_button(event.gbutton);
        break;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX)
            gaxis_left_x = event.gaxis.value;
        else if (event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY)
            gaxis_left_y = event.gaxis.value;
        break;
    case SDL_EVENT_KEY_UP:
    case SDL_EVENT_KEY_DOWN:
        process_key(event.key);
    }
}

int game_init(BackBuffer *back_buffer, AudioBuffer *audio_buffer) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC)) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (!SDL_CreateWindowAndRenderer(
        "Handmade Hero",
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_BORDERLESS,
        &window,
        &renderer
    )) {
        printf("Couldn't create window/renderer: %s\n", SDL_GetError());
        goto end1;
    }

    SDL_GetWindowSize(window, (int*)&back_buffer->width, (int*)&back_buffer->height);
    back_buffer->pitch = back_buffer->width * BYTES_PER_PIXEL;
    if (!(back_buffer->texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBX32,
        SDL_TEXTUREACCESS_STREAMING,
        back_buffer->width,
        back_buffer->height
    ))) {
        printf("Couldn't create back_buffer texture: %s\n", SDL_GetError());
        goto end2;
    }

    back_buffer->memory = malloc(back_buffer->width*back_buffer->height * (uint32_t)BYTES_PER_PIXEL);
    if (!back_buffer->memory) {
        printf("Couldn't allocate memory for back buffer.\n");
        goto end3;
    }

    audio_buffer->len = (AUDIO_SAMPLE_RATE * sizeof(int16_t)*2);
    audio_buffer->memory = malloc(audio_buffer->len);
    if (!audio_buffer->memory) {
        printf("Couldn't allocate memory for audio buffer.\n");

        end3:
        SDL_DestroyTexture(back_buffer->texture);
        end2:
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        end1:
        SDL_Quit();
        return 1;
    }
    audio_buffer->running_sample = 0;
    audio_buffer->volume = 2000;
    audio_buffer->samples = AUDIO_SAMPLE_RATE;
    audio_buffer->hz = 256;
    audio_buffer->period = (float)AUDIO_SAMPLE_RATE / (float)audio_buffer->hz;

    return 0;
}

void game_tick(void) {
    if (right.is_down) ++x_offset;
    if (left.is_down) --x_offset;
    if (down.is_down) ++y_offset;
    if (up.is_down) --y_offset;
    if (right.is_down || left.is_down || down.is_down || up.is_down)
        SDL_RumbleGamepad(gamepad, 0, 0xffff, 10);

    if (gaxis_left_x > GAMEPAD_STICK_THRESHOLD && !right.is_down) ++x_offset;
    if (gaxis_left_x < -GAMEPAD_STICK_THRESHOLD && !left.is_down) --x_offset;
    if (gaxis_left_y > GAMEPAD_STICK_THRESHOLD && !down.is_down) ++y_offset;
    if (gaxis_left_y < -GAMEPAD_STICK_THRESHOLD && !up.is_down) --y_offset;
}

int main(void) {
    BackBuffer back_buffer = {0};
    AudioBuffer audio_buffer = {0};
    if (game_init(&back_buffer, &audio_buffer)) return 1;

    is_running = true;
    while (is_running) {
        SDL_RenderClear(renderer);

        SDL_Event event;
        while (SDL_PollEvent(&event)) process_event(event, &back_buffer);

        game_tick();

        render_back_buffer(&back_buffer);
        fill_audio_buffer(&audio_buffer);

        SDL_FRect frect = {0, 0, back_buffer.width, back_buffer.height};
        SDL_RenderTexture(renderer, back_buffer.texture, &frect, &frect);
        SDL_RenderPresent(renderer);
    }

    free(back_buffer.memory);
    free(audio_buffer.memory);

    if (audio_stream) SDL_DestroyAudioStream(audio_stream);
    if (gamepad) SDL_CloseGamepad(gamepad);

    SDL_DestroyTexture(back_buffer.texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}
