#include "render.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define VENTANA_ANCHO 1024
#define VENTANA_ALTO   768

/* helper usado en render_hud, necesita ser static o definido antes */
static float normalizar_angulo(float a) {
    while (a >  (float)M_PI) a -= 2.0f * (float)M_PI;
    while (a < -(float)M_PI) a += 2.0f * (float)M_PI;
    return a;
}
/*
 * proyectar: convierte un punto 3D (en espacio de cámara) a píxeles.
 * Devuelve false si el punto está detrás de la cámara (z <= 1).
 *
 * Transformación:
 *   - Centro de pantalla = (ANCHO/2, ALTO/2)
 *   - Escala vertical cubre exactamente el rango [-1,1] en y
 *   - Eje y de SDL apunta hacia abajo, por eso restamos
 *   - Gran angular: usamos el alto para escalar x también
 */
static bool proyectar(float vx, float vy, float vz, int *sx, int *sy) {
    if (vz <= 1.0f) return false;

    float escala = (float)VENTANA_ALTO / 2.0f;

    *sx = VENTANA_ANCHO / 2 + (int)(escala * vx / vz);
    *sy = VENTANA_ALTO  / 2 - (int)(escala * vy / vz);
    return true;
}

/*
 * Pasos:
 *  1. Rotar el vértice según el yaw del objeto (lo pone mirando bien)
 *  2. Trasladar al lugar del objeto en el mundo
 *  3. Restar posición de la cámara (mueve el mundo relativo al jugador)
 *  4. Rotar según yaw de la cámara (giro del jugador)
 *
 * Nota: los coords del STL vienen como x,z,y (el eje z del modelo es
 * la altura visual), por eso intercambiamos coords[1] y coords[2].
 */
static void transformar_vertice(
const modelo_t *mesh, size_t idx,
float obj_x, float obj_y, float obj_z, float obj_yaw,
float cam_x, float cam_y, float cam_z, float cam_yaw,
float *out_x, float *out_y, float *out_z)
{
    float vx =  mesh->coords[idx * 3];
    float vy =  mesh->coords[idx * 3 + 2];  /* Z del STL = altura visual */
    float vz =  mesh->coords[idx * 3 + 1];  /* Y del STL = profundidad   */

    /* 1. rotar por yaw del objeto */
    float c = cosf(obj_yaw), s = sinf(obj_yaw);
    float rx = vx * c + vz * s;
    float rz = -vx * s + vz * c;

    /* 2. trasladar al mundo */
    float wx = rx + obj_x;
    float wy = vy + obj_y;
    float wz = rz + obj_z;

    /* 3. restar cámara */
    wx -= cam_x;
    wy -= cam_y;
    wz -= cam_z;

    /* 4. rotar por yaw de cámara (negativo: compensamos el giro del jugador) */
    float cc = cosf(-cam_yaw), ss = sinf(-cam_yaw);
    *out_x = wx * cc + wz * ss;
    *out_y = wy;
    *out_z = -wx * ss + wz * cc;
}


static void dibujar_mesh(
    SDL_Renderer *renderer,
    const modelo_t *mesh,
    float obj_x, float obj_y, float obj_z, float obj_yaw,
    float cam_x, float cam_y, float cam_z, float cam_yaw)
{
    if (mesh == NULL) return;

    for (size_t i = 0; i < mesh->nlineas; i++) {
        size_t a = mesh->lineas[i * 2];
        size_t b = mesh->lineas[i * 2 + 1];

        float ax, ay, az, bx, by, bz;
        transformar_vertice(mesh, a,
            obj_x, obj_y, obj_z, obj_yaw,
            cam_x, cam_y, cam_z, cam_yaw,
            &ax, &ay, &az);
        transformar_vertice(mesh, b,
            obj_x, obj_y, obj_z, obj_yaw,
            cam_x, cam_y, cam_z, cam_yaw,
            &bx, &by, &bz);

        int x1, y1, x2, y2;
        if (!proyectar(ax, ay, az, &x1, &y1)) continue;
        if (!proyectar(bx, by, bz, &x2, &y2)) continue;

        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    }
}

/*
 * dibujar_char_hud: dibuja un carácter del STL en coordenadas de pantalla.
 * Los modelos de letras están en el plano XY centrados en el origen,
 * los escalamos y desplazamos directamente en pantalla (sin perspectiva).
 */
static void dibujar_char_hud(
    SDL_Renderer *renderer,
    const modelo_t *mesh,
    int px, int py, float escala)
{
    if (mesh == NULL) return;

    for (size_t i = 0; i < mesh->nlineas; i++) {
        size_t a = mesh->lineas[i * 2];
        size_t b = mesh->lineas[i * 2 + 1];

        float ax = mesh->coords[a * 3]     * escala + (float)px;
        float ay = -mesh->coords[a * 3 + 1] * escala + (float)py;
        float bx = mesh->coords[b * 3]     * escala + (float)px;
        float by = -mesh->coords[b * 3 + 1] * escala + (float)py;

        SDL_RenderDrawLine(renderer,
            (int)ax, (int)ay,
            (int)bx, (int)by);
    }
}

/*
 * dibujar_texto_hud: dibuja una cadena de caracteres en pantalla.
 * Cada carácter ocupa 'tam' píxeles de ancho.
 */
static void dibujar_texto_hud(
    SDL_Renderer *renderer,
    lista_modelos_t *modelos,
    const char *texto,
    int px, int py, int tam)
{
    char nombre[2] = {0, 0};
    for (int i = 0; texto[i] != '\0'; i++) {
        nombre[0] = texto[i];
        modelo_t *m = lista_modelos_buscar(modelos, nombre);
        if (m != NULL)
            dibujar_char_hud(renderer, m, px + i * tam, py, (float)tam / 2.0f);
    }
}

typedef struct {
    float x, y, z;
    float yaw;
} cam_t;

static cam_t calcular_camara(const mundo_t *mundo) {
    return (cam_t){
        .x   = mundo->jugador.pos.x,
        .y   = ALTURA_CAMARA,
        .z   = mundo->jugador.pos.z,
        .yaw = mundo->jugador.yaw
    };
}

static void render_escena(SDL_Renderer *renderer, const mundo_t *mundo) {
    cam_t cam = calcular_camara(mundo);

    lista_modelos_t *m = mundo->modelos;

    /* color de escena: blanco (fondo) */
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    /* fondo: horizonte, montaña, luna */
    modelo_t *horizonte = lista_modelos_buscar(m, "HORIZONTE");
    modelo_t *montana   = lista_modelos_buscar(m, "MONTANA");
    modelo_t *luna      = lista_modelos_buscar(m, "LUNA");

    dibujar_mesh(renderer, horizonte, 0,0,0,0, cam.x,cam.y,cam.z,cam.yaw);
    dibujar_mesh(renderer, montana,   0,0,0,0, cam.x,cam.y,cam.z,cam.yaw);
    dibujar_mesh(renderer, luna,      0,0,0,0, cam.x,cam.y,cam.z,cam.yaw);

    // Color de Obstaculos: verde (obstaculos)
    SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF);

    /* obstáculos */
    for (int i = 0; i < CANT_OBSTACULOS; i++) {
        if (mundo->obstaculos[i].destruido) continue;
        const obstaculo_t *o = &mundo->obstaculos[i];
        dibujar_mesh(renderer, o->mesh,
            o->pos.x, 0.0f, o->pos.z, o->yaw,
            cam.x, cam.y, cam.z, cam.yaw);
    }

    /* color principal: blanco (tanques y misiles) */
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    modelo_t *mesh_tanque  = lista_modelos_buscar(m, "TANQUE");
    modelo_t *mesh_torreta = lista_modelos_buscar(m, "TORRETA");
    modelo_t *mesh_radar   = lista_modelos_buscar(m, "RADAR");
    modelo_t *mesh_misil   = lista_modelos_buscar(m, "MISIL");

    /* enemigo */
    if (mundo->enemigo.vivo) {
        const tanque_t *e = &mundo->enemigo;
        dibujar_mesh(renderer, mesh_tanque,
            e->pos.x, 0.0f, e->pos.z, e->yaw,
            cam.x, cam.y, cam.z, cam.yaw);
        dibujar_mesh(renderer, mesh_torreta,
            e->pos.x, 3.0f, e->pos.z, e->yaw + e->yaw_torreta,
            cam.x, cam.y, cam.z, cam.yaw);
        dibujar_mesh(renderer, mesh_radar,
            e->pos.x, 3.0f, e->pos.z, e->yaw + e->yaw_torreta,
            cam.x, cam.y, cam.z, cam.yaw);
    }

    /* misil del jugador */
    if (mundo->jugador.misil.activo) {
        const misil_t *mi = &mundo->jugador.misil;
        dibujar_mesh(renderer, mesh_misil,
            mi->pos.x, 0.0f, mi->pos.z, mi->yaw,
            cam.x, cam.y, cam.z, cam.yaw);
    }

    /* misil del enemigo */
    if (mundo->enemigo.misil.activo) {
        const misil_t *mi = &mundo->enemigo.misil;
        dibujar_mesh(renderer, mesh_misil,
            mi->pos.x, 0.0f, mi->pos.z, mi->yaw,
            cam.x, cam.y, cam.z, cam.yaw);
    }
}

static void render_hud(SDL_Renderer *renderer, const mundo_t *mundo) {
    SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);

    lista_modelos_t *m = mundo->modelos;
    const int TAM_CHAR = 24;

    /* vidas: una * por cada vida restante */
    modelo_t *vida_ico = lista_modelos_buscar(m, "*");
    for (int i = 0; i < mundo->vidas; i++) {
        dibujar_char_hud(renderer, vida_ico,
            20 + i * (TAM_CHAR + 30), 20, (float)TAM_CHAR);
    }

    /* puntaje */
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", mundo->puntaje);
    dibujar_texto_hud(renderer, m, buf,
        VENTANA_ANCHO - (int)strlen(buf) * (TAM_CHAR + 2) - 20, 20, TAM_CHAR);

    /* mira: + si el enemigo está a menos de 0.15 rad, - si no */
    float ang_enemigo = 0.0f;
    if (mundo->enemigo.vivo) {
        float dx = mundo->enemigo.pos.x - mundo->jugador.pos.x;
        float dz = mundo->enemigo.pos.z - mundo->jugador.pos.z;
        float ang_abs = atan2f(dx, dz);
        ang_enemigo = normalizar_angulo(ang_abs - mundo->jugador.yaw);

        /* dirección del enemigo cuando está fuera del rango de visión */
        if (fabsf(ang_enemigo) > 1.0f) {
            const char *msg = NULL;
            if      (ang_enemigo >  2.44f || ang_enemigo < -2.44f) msg = "DETRAS";
            else if (ang_enemigo < 1.0f)                          msg = "IZQUIERDA";
            else                                                    msg = "DERECHA";
            if (msg)
                dibujar_texto_hud(renderer, m, msg,
                    VENTANA_ANCHO/2 - (int)(strlen(msg)/2) * TAM_CHAR,
                    VENTANA_ALTO - 60, TAM_CHAR);
        }
    }

    const char *mira = (fabsf(ang_enemigo) < 0.15f) ? "+" : "-";
    modelo_t *mesh_mira = lista_modelos_buscar(m, mira);
    dibujar_char_hud(renderer, mesh_mira,
        VENTANA_ANCHO/2, VENTANA_ALTO/2, (float)TAM_CHAR * 2);
}


static void render_anim_jugador(SDL_Renderer *renderer, const mundo_t *mundo) {
    SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);

    modelo_t *hash = lista_modelos_buscar(mundo->modelos, "#");
    if (hash == NULL) return;

    int lineas_a_dibujar = mundo->animacion.linea_hud;
    if (lineas_a_dibujar > (int)hash->nlineas)
        lineas_a_dibujar = (int)hash->nlineas;

    /* dibujamos solo las líneas hasta la actual (efecto de vidrio rompiéndose) */
    for (int i = 0; i < lineas_a_dibujar; i++) {
        size_t a = hash->lineas[i * 2];
        size_t b = hash->lineas[i * 2 + 1];

        float escala = (float)VENTANA_ALTO / 2.0f;
        int ax = VENTANA_ANCHO/2 + (int)(hash->coords[a*3]     * escala);
        int ay = VENTANA_ALTO /2 - (int)(hash->coords[a*3 + 1] * escala);
        int bx = VENTANA_ANCHO/2 + (int)(hash->coords[b*3]     * escala);
        int by = VENTANA_ALTO /2 - (int)(hash->coords[b*3 + 1] * escala);

        SDL_RenderDrawLine(renderer, ax, ay, bx, by);
    }
}

#define G 9.81f

static void render_anim_enemigo(SDL_Renderer *renderer, const mundo_t *mundo) {
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    lista_modelos_t *m = mundo->modelos;
    cam_t cam = calcular_camara(mundo);

    float t = mundo->animacion.tiempo;
    vec2_t pos_explosion = mundo->animacion.pos;

    modelo_t *piezas[] = {
        lista_modelos_buscar(m, "TORRETA"),
        lista_modelos_buscar(m, "RADAR"),
        lista_modelos_buscar(m, "RESTO1"),
        lista_modelos_buscar(m, "RESTO1"),
        lista_modelos_buscar(m, "RESTO2"),
        lista_modelos_buscar(m, "RESTO2"),
    };
    const int cant_piezas = 6;

    for (int i = 0; i < cant_piezas; i++) {
        /* cada pieza sale en una dirección diferente (60° entre sí) */
        float angulo_base = (float)i * (2.0f * (float)M_PI / cant_piezas);

        /* tiro oblicuo: vel_x = 5 m/s, vel_z = 10 m/s, g en z */
        float vx = 5.0f;
        float vz = 10.0f;

        float dx = vx * t * cosf(angulo_base);
        float dz = vz * t - 0.5f * G * t * t;  /* altura */
        float dy = vx * t * sinf(angulo_base);

        /* rotación propia de cada pieza para que gire mientras vuela */
        float rot_pieza = angulo_base + t * 2.0f;

        float px = pos_explosion.x + dx;
        float py = dz;                   /* dz es la altura (y en el render) */
        float pz = pos_explosion.z + dy;

        dibujar_mesh(renderer, piezas[i],
            px, py, pz, rot_pieza,
            cam.x, cam.y, cam.z, cam.yaw);
    }
}

void render_mundo(SDL_Renderer *renderer, const mundo_t *mundo) {
    /* limpiar pantalla */
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(renderer);

    render_escena(renderer, mundo);
    render_hud(renderer, mundo);

    /* animaciones encima de todo */
    if (mundo->animacion.tipo == ANIM_JUGADOR)
        render_anim_jugador(renderer, mundo);
    else if (mundo->animacion.tipo == ANIM_ENEMIGO)
        render_anim_enemigo(renderer, mundo);
}

