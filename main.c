#include <SDL2/SDL.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "lista_modelos.h"
#include "mundo.h"
#include "render.h"

#define VENTANA_ANCHO 1024
#define VENTANA_ALTO   768
#define JUEGO_FPS       24

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    srand((unsigned int)time(NULL));

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer(VENTANA_ANCHO, VENTANA_ALTO, 0, &window, &renderer);
    SDL_SetWindowTitle(window, "Battle Zone");

    /* cargar modelos */
    lista_modelos_t *modelos = lista_modelos_crear();
    if (modelos == NULL) {
        fprintf(stderr, "Error: no se pudo crear la lista de modelos\n");
        return 1;
    }
    if (!lista_modelos_cargar(modelos, "../modelos.stl")) {
        fprintf(stderr, "Error: no se pudo cargar modelos.stl\n");
        lista_modelos_destruir(modelos);
        return 1;
    }

    /* crear mundo */
    mundo_t *mundo = mundo_crear(modelos);
    if (mundo == NULL) {
        fprintf(stderr, "Error: no se pudo crear el mundo\n");
        lista_modelos_destruir(modelos);
        return 1;
    }

    SDL_Event event;
    unsigned int ticks_prev = SDL_GetTicks();
    bool corriendo = true;

    while (corriendo) {
        /* procesar eventos */
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                corriendo = false;
        }

        /* leer teclado */
        const Uint8 *keys = SDL_GetKeyboardState(NULL);

        if (keys[SDL_SCANCODE_W]) mundo_jugador_mover(mundo,  1);
        if (keys[SDL_SCANCODE_S]) mundo_jugador_mover(mundo, -1);
        if (keys[SDL_SCANCODE_A]) mundo_jugador_girar(mundo, -1);
        if (keys[SDL_SCANCODE_D]) mundo_jugador_girar(mundo,  1);
        if (keys[SDL_SCANCODE_SPACE]) mundo_jugador_disparar(mundo);

        /* calcular delta time */
        unsigned int ticks_ahora = SDL_GetTicks();
        float dt = (float)(ticks_ahora - ticks_prev) / 1000.0f;
        ticks_prev = ticks_ahora;

        /* actualizar lógica */
        mundo_actualizar(mundo, dt);

        if (mundo_terminado(mundo))
            corriendo = false;

        /* dibujar */
        render_mundo(renderer, mundo);
        SDL_RenderPresent(renderer);

        /* FPS cap */
        unsigned int frame_ms = SDL_GetTicks() - ticks_ahora;
        if (frame_ms < 1000 / JUEGO_FPS)
            SDL_Delay(1000 / JUEGO_FPS - frame_ms);
        else
            fprintf(stderr, "Perdiendo cuadros\n");
    }

    /* liberar */
    mundo_destruir(mundo);
    lista_modelos_destruir(modelos);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}