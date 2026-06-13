#include <SDL2/SDL.h>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "ej3.h"



#define VENTANA_ANCHO 1024
#define VENTANA_ALTO 768

#define JUEGO_FPS 24

//cuzzi-codigo starts

typedef struct {
    float x;
    float y;
    float z;
} Vec3;

typedef struct {
    char nombre[32];

    size_t ncoords;
    float *coords;

    size_t nlineas;
    size_t *lineas;
} Mesh;

typedef struct {
    float x;
    float y;
    float z;

    float yaw;
} Camera;

typedef struct {
    Mesh *mesh;

    float x;
    float y;
    float z;

    float yaw;
} Entity;

#define CANT_OBSTACULOS 50

Entity obstaculos[CANT_OBSTACULOS];


Mesh cargar_modelo(const char *archivo, const char *nombre_buscado) {
    FILE *f = fopen(archivo, "rb");

    if (!f) {
        perror("fopen");
        exit(1);
    }

    leer_encabezado_stl(f);

    unidades_t unidades;
    size_t maxlong;

    leer_formato_stl(f,&unidades,&maxlong);

    char *etiqueta = malloc(maxlong);

    size_t ncoords;
    float *coords;

    size_t nlineas;
    size_t *lineas;

    while (leer_modelo_3d(f,maxlong,etiqueta,&ncoords,&coords,&nlineas,&lineas)) {
        if (strcmp(etiqueta, nombre_buscado) == 0) {
            Mesh m;
            strcpy(m.nombre,etiqueta);

            m.ncoords = ncoords;
            m.coords = coords;

            m.nlineas = nlineas;
            m.lineas = lineas;

            free(etiqueta);
            fclose(f);

            return m;
        }

        free(coords);
        free(lineas);
    }
    printf("nombre no en archivo %s\n",nombre_buscado);
    exit(1);
}

bool proyectar(Vec3 v, int *sx, int *sy) {

    if (v.z <= 1.0f)
        return false;

    float focal = 500.0f;

    *sx = VENTANA_ANCHO/2 +
          (int)(focal * v.x / v.z);

    *sy = VENTANA_ALTO/2 -
          (int)(focal * v.y / v.z);

    return true;
}

Vec3 mesh_vertex(const Mesh *mesh, size_t idx) {
    return (Vec3){
        mesh->coords[idx * 3],
        mesh->coords[idx * 3 + 2],
        mesh->coords[idx * 3 + 1]
    };
}

void render_mesh(SDL_Renderer *renderer,const Mesh *mesh, float px,float py, float pz,float yaw,const Camera *cam) {
    float c = cosf(yaw);
    float s = sinf(yaw);
    for (size_t i=0; i<mesh -> nlineas; i++) {
        size_t a = mesh -> lineas[i*2];
        size_t b = mesh -> lineas[i*2 + 1];

        Vec3 va = mesh_vertex(mesh, a);
        Vec3 vb = mesh_vertex(mesh, b);

        float ax = va.x * c + va.z * s;
        float az = -va.x * s + va.z * c;

        float bx = vb.x * c + vb.z * s;
        float bz = -vb.x * s + vb.z * c;

        va.x = ax + px;
        va.y = va.y + py;
        va.z = az + pz;

        vb.x = bx + px;
        vb.y = vb.y + py;
        vb.z = bz + pz;

        va.x -= cam->x;
        va.y -= cam->y;
        va.z -= cam->z;

        vb.x -= cam->x;
        vb.y -= cam->y;
        vb.z -= cam->z;

        float cc = cosf(-cam->yaw);
        float ss = sinf(-cam->yaw);

        float tx = va.x * cc + va.z * ss;
        float tz = -va.x * ss + va.z * cc;

        va.x = tx;
        va.z = tz;

        tx = vb.x * cc + vb.z * ss;
        tz = -vb.x * ss + vb.z * cc;

        vb.x = tx;
        vb.z = tz;

        int x1,y1;
        int x2,y2;

        if (!proyectar(va,&x1,&y1))
            continue;

        if (!proyectar(vb,&x2,&y2))
            continue;

        SDL_RenderDrawLine(
            renderer,
            x1,y1,
            x2,y2);
    }
}

void render_entity(SDL_Renderer *renderer, const Entity *e, const Camera *cam) {
    render_mesh(
        renderer,
        e->mesh,
        e->x,
        e->y,
        e->z,
        e->yaw,
        cam
    );
}



#define VELOCIDAD 10.0f
#define VELOCIDAD_GIRO 0.1f

//Cuzzi-codigo ends




int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;

    SDL_CreateWindowAndRenderer(VENTANA_ANCHO, VENTANA_ALTO, 0, &window, &renderer);
    SDL_SetWindowTitle(window, "Battle Zone");

    int dormir = 0;


    //cuzzi-codigo begins
    Mesh tanque   = cargar_modelo("../modelos.stl", "TANQUE");
    Mesh misil    = cargar_modelo("../modelos.stl", "MISIL");
    Mesh torreta  = cargar_modelo("../modelos.stl", "TORRETA");
    Mesh montana  = cargar_modelo("../modelos.stl", "MONTANA");
    Mesh luna  = cargar_modelo("../modelos.stl", "LUNA");
    Mesh horizonte  = cargar_modelo("../modelos.stl", "HORIZONTE");


    Mesh cubo1  = cargar_modelo("../modelos.stl", "CUBO1");
    Mesh cubo2  = cargar_modelo("../modelos.stl", "CUBO2");
    Mesh cubo3  = cargar_modelo("../modelos.stl", "CUBO3");

    Mesh piramide1  = cargar_modelo("../modelos.stl", "PIRAMIDE1");
    Mesh piramide2  = cargar_modelo("../modelos.stl", "PIRAMIDE2");
    Mesh piramide3  = cargar_modelo("../modelos.stl", "PIRAMIDE3");


    Mesh *tipos_obstaculos[] = {
        &cubo1,
        &cubo2,
        &cubo3,
        &piramide1,
        &piramide2,
        &piramide3
    };

    const int cantidad_tipos = 6;

    Entity jugador = {
        .mesh = &tanque,
        .x = 0,
        .y = 0,
        .z = 150,
        .yaw = 0
    };

    Camera cam = {
        .x = 0,
        .y = 3,
        .z = 0,
        .yaw = 0
    };

    srand((unsigned int)time(NULL));

    for (int i= 0; i<CANT_OBSTACULOS; i++) {
        int tipo = rand() % cantidad_tipos;
        obstaculos[i].mesh = tipos_obstaculos[tipo];

        obstaculos[i].x = rand() % 300 - 150;
        obstaculos[i].z = rand() % 300 - 150;
        obstaculos[i].y = 0;

        obstaculos[i].yaw = ((float)rand() / RAND_MAX * 2.0f * M_PI);

    }

    Entity enemigo = {
        .mesh = &tanque,
        .x = 0,
        .y = 0,
        .z = 0,
        .yaw = 0
    };

    Entity montaña1 = {
        .mesh = &montana,
        .x = 0,
        .y = 0,
        .z = 0,
        .yaw = 0
    };
    Entity luna1 = {
        .mesh = &luna,
        .x = 0,
        .y = 0,
        .z = 0,
        .yaw = 0
    };
    Entity horizonte1 = {
        .mesh = &horizonte,
        .x = 0,
        .y = 0,
        .z = 0,
        .yaw = 0
    };

    Entity misil1 = {
        .mesh = &misil,
        .x = 100,
        .y = 0,
        .z = 500,
        .yaw = 0
    };
    //cuzzi-codigo ends

    // BEGIN código del alumno
    unsigned char color_hud[3] = {0xff, 0, 0};
    unsigned char color_principal[3] = {0xff, 0xff, 0xff};
    unsigned char color_escena[3] = {0, 0xff, 0};
    int x = VENTANA_ANCHO / 2;
    int y = VENTANA_ALTO / 2;
    // END código del alumno

    unsigned int ticks = SDL_GetTicks();
    while(1) {
        if(SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                break;

            // BEGIN código del alumno
            const Uint8 *keys = SDL_GetKeyboardState(NULL);
            if (keys[SDL_SCANCODE_W]){
                cam.x += sinf(cam.yaw) * VELOCIDAD;
                cam.z += cosf(cam.yaw) * VELOCIDAD;
            }

            if (keys[SDL_SCANCODE_S]) {
                cam.x -= sinf(cam.yaw) * VELOCIDAD;
                cam.z -= cosf(cam.yaw) * VELOCIDAD;
            }

            if (keys[SDL_SCANCODE_A])
                cam.yaw -= VELOCIDAD_GIRO;

            if (keys[SDL_SCANCODE_D])
                cam.yaw += VELOCIDAD_GIRO;

            // END código del alumno

            continue;
        }

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0x00);

        // BEGIN código del alumno

        //Escenario - 50 Obstaculos - 2 montañas (pinto)
        SDL_SetRenderDrawColor(renderer, color_escena[0], color_escena[1], color_escena[2], 0x00);

        for (int i = 0; i < CANT_OBSTACULOS; i++) {
            render_entity(
                renderer,
                &obstaculos[i],
                &cam
            );
        }
        //Entities - jugador - misil - enemigo
        SDL_SetRenderDrawColor(renderer, color_principal[0], color_principal[1], color_principal[2], 0x00);

        //mapa begin
        render_entity(renderer, &montaña1, &cam);
        render_entity(renderer, &luna1, &cam);
        render_entity(renderer, &horizonte1, &cam);
        //mapa ends

        render_entity(renderer, &enemigo, &cam);

        render_entity(renderer, &misil1, &cam);

        // END código del alumno

        SDL_RenderPresent(renderer);
        ticks = SDL_GetTicks() - ticks;
        if(dormir) {
            SDL_Delay(dormir);
            dormir = 0;
        }
        else if(ticks < 1000 / JUEGO_FPS)
            SDL_Delay(1000 / JUEGO_FPS - ticks);
        else
            printf("Perdiendo cuadros\n");
        ticks = SDL_GetTicks();
    }

    // BEGIN código del alumno
    // END código del alumno

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}
