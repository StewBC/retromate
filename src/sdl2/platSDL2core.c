/*
 *  platSDL2core.c
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL.h>

#include "../global.h"

#include "chess_pieces.h"
#include "platSDL2.h"
#include "proggyclean_font.h"

/*-----------------------------------------------------------------------*/
static void plat_load_assets_from_memory(void) {
    SDL_RWops *rw = SDL_RWFromConstMem(pieces_png, pieces_png_len);
    SDL_Surface *surface = IMG_LoadPNG_RW(rw);
    if (!surface) {
        exit(1);
    }

    sdl.piece_texture = SDL_CreateTextureFromSurface(sdl.renderer, surface);
    SDL_FreeSurface(surface);

    if (!sdl.piece_texture) {
        exit(1);
    }
}

/*-----------------------------------------------------------------------*/
void plat_core_active_term(bool active) {
    plat_draw_clrscr();
    if (active) {
        global.view.terminal_active = 1;
        sdl.draw_color = COLOR_BLACK;
        sdl.text_bg_color = COLOR_GREEN;
    } else {
        global.view.terminal_active = 0;
        global.view.refresh = 1;
    }
}

/*-----------------------------------------------------------------------*/
void plat_core_copy_ascii_to_display(void *dest, const void *src, size_t n) {
    memcpy(dest, src, n);
}

/*-----------------------------------------------------------------------*/
void plat_core_exit() {
    exit(1);
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_get_cols(void) {
    return SCREEN_TEXT_WIDTH;
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_get_rows(void) {
    return SCREEN_TEXT_HEIGHT;
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_get_status_x(void) {
    // The accoutrements (1..8) + board + 1 extra space
    return 2 + SQUARE_TEXT_WIDTH * 8;
}

/*-----------------------------------------------------------------------*/
void plat_core_init(void) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_RWops *rw = SDL_RWFromConstMem(ProggyClean_ttf, ProggyClean_ttf_len);
    sdl.font = TTF_OpenFontRW(rw, 1, CHARACTER_HEIGHT);
    if (!sdl.font) {
        exit(1);
    }

    // int w;
    // TTF_SizeText(sdl.font, "W", &w, NULL);
    // assert(CHARACTER_WIDTH == w);

    sdl.window = SDL_CreateWindow("RetroMate",
                                  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  SCREEN_DISPLAY_WIDTH, SCREEN_DISPLAY_HEIGHT,
                                  SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

    sdl.renderer = SDL_CreateRenderer(sdl.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Use a texture as the "background" - this works with the 8-bit design of
    // drawing as little as possible - the texture captures the cumulative drawing
    // over frames
    sdl.framebuffer = SDL_CreateTexture(
                          sdl.renderer,
                          SDL_PIXELFORMAT_RGBA8888,
                          SDL_TEXTUREACCESS_TARGET,
                          SCREEN_DISPLAY_WIDTH,
                          SCREEN_DISPLAY_HEIGHT
                      );
    if (!sdl.framebuffer) {
        printf("error: framebuffer texture is null\n");
        exit(1);
    }
    // Set the texture as the target for drawing commands
    SDL_SetRenderTarget(sdl.renderer, sdl.framebuffer);

    plat_load_assets_from_memory();
    SDL_StartTextInput();

    // Set the cursor
    global.view.cursor_char[0] = -128;
    global.view.cursor_char[2] = -128;

    // Show the title screen
    plat_draw_splash_screen();

    // Clear the title screen
    plat_draw_board();
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_key_input(input_event_t *evt) {
    SDL_Event e;

    evt->code = INPUT_NONE;

    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                evt->code = INPUT_QUIT;
                return 1;
                break;

            case SDL_MOUSEMOTION:
                evt->code = INPUT_MOUSE_MOVE;
                evt->mouse_x = e.motion.x;
                evt->mouse_y = e.motion.y;
                evt->key_value = 0;
                // don't return, keep looping "eating" updates
                break;

            case SDL_MOUSEBUTTONUP:
                switch (e.button.button) {
                    case SDL_BUTTON_LEFT:
                        evt->code = INPUT_MOUSE_CLICK;
                        evt->mouse_x = e.button.x;
                        evt->mouse_y = e.button.y;
                        evt->key_value = 0;
                        break;

                    case SDL_BUTTON_RIGHT:
                        evt->code = INPUT_BACK;
                        break;
                }
                return 1;
                break;

            case SDL_TEXTINPUT:
                evt->code = INPUT_KEY;
                evt->key_value = (uint8_t)e.text.text[0];  // first character only
                return 1;
                break;

            case SDL_KEYDOWN: {
                SDL_Keycode k = e.key.keysym.sym;
                switch (k) {
                    case SDLK_ESCAPE:
                        evt->code = INPUT_BACK;
                        return 1;

                    case SDLK_RETURN:
                        evt->code = INPUT_SELECT;
                        return 1;

                    case SDLK_LEFT:
                        evt->code = INPUT_LEFT;
                        return 1;

                    case SDLK_RIGHT:
                        evt->code = INPUT_RIGHT;
                        return 1;

                    case SDLK_UP:
                        evt->code = INPUT_UP;
                        return 1;

                    case SDLK_DOWN:
                        evt->code = INPUT_DOWN;
                        return 1;

                    case SDLK_TAB:
                        evt->code = INPUT_VIEW_TOGGLE;
                        return 1;

                    case SDLK_BACKSPACE:
                        evt->code = INPUT_BACKSPACE;
                        return 1;

                    case SDLK_s:
                        if (e.key.keysym.mod & KMOD_CTRL) {
                            evt->code = INPUT_SAY;
                            return 1;
                        }
                }
                break;
            }
        }
    }

    if (evt->code != INPUT_NONE) {
        return 1;
    }

    return 0;
}

/*-----------------------------------------------------------------------*/
void plat_core_key_wait_any() {
    // Wait for quiescence
    do {
        plat_core_key_input(&global.os.input_event);
    } while (!(global.os.input_event.code == INPUT_NONE || global.os.input_event.code == INPUT_MOUSE_MOVE));

    // Now wait for a key
    do {
        plat_core_key_input(&global.os.input_event);
    } while (global.os.input_event.code == INPUT_NONE || global.os.input_event.code == INPUT_MOUSE_MOVE);
}

/*-----------------------------------------------------------------------*/
void plat_core_log_free_mem(char *mem) {
    free(mem);
}

/*-----------------------------------------------------------------------*/
char *plat_core_log_malloc(unsigned int size) {
    return malloc(size);
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_mouse_to_cursor(void) {
    int x = global.os.input_event.mouse_x - BOARD_START_X;
    int y = global.os.input_event.mouse_y - BOARD_START_Y;

    if (x >= 0 && x < BOARD_DISPLAY_WIDTH) {
        if (y >= 0 && y < BOARD_DISPLAY_HEIGHT) {
            return (y / SQUARE_DISPLAY_HEIGHT) * 8 + (x / SQUARE_DISPLAY_WIDTH);
        }
    }

    return MENU_SELECT_NONE;
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_mouse_to_menu_item(void) {
    uint8_t x = global.os.input_event.mouse_x / CHARACTER_WIDTH;
    uint8_t y = global.os.input_event.mouse_y / CHARACTER_HEIGHT;

    if (x > global.view.mc.x && x < global.view.mc.x + global.view.mc.w - 1) {
        uint8_t item_start_y = global.view.mc.y + 2;
        uint8_t menu_bottom = global.view.mc.y + global.view.mc.h - 2;

        if (y < item_start_y || y > menu_bottom) {
            return MENU_SELECT_NONE;
        }

        return y - item_start_y;
    }

    return MENU_SELECT_NONE;
}

/*-----------------------------------------------------------------------*/
void plat_core_shutdown(void) {
    SDL_StopTextInput();

    if (sdl.font) {
        TTF_CloseFont(sdl.font);
    }
    TTF_Quit();

    if(sdl.framebuffer) {
        SDL_DestroyTexture(sdl.framebuffer);
    }

    if (sdl.renderer) {
        SDL_DestroyRenderer(sdl.renderer);
    }

    if (sdl.window) {
        SDL_DestroyWindow(sdl.window);
    }

    SDL_Quit();
}
