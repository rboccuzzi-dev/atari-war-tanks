#include "mundo.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

//ALGREBRA 💀
static float distancia(vec2_t a, vec2_t b) {
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return sqrtf(dx*dx + dz*dz);
}

/* Ángulo desde 'desde' hacia 'hacia', en radianes */
static float angulo_hacia(vec2_t desde, vec2_t hacia) {
    return atan2f(hacia.x - desde.x, hacia.z - desde.z);
}

/* Normaliza un ángulo al rango [-PI, PI] */
static float normalizar_angulo(float a) {
    while (a >  (float)M_PI) a -= 2.0f * (float)M_PI;
    while (a < -(float)M_PI) a += 2.0f * (float)M_PI;
    return a;
}

/* ── posición libre para crear al enemigo ───────────────────────────── */

static vec2_t posicion_enemigo(const mundo_t *mundo) {
    vec2_t pos;
    int intentos = 0;
    do {
        float angulo = ((float)rand() / RAND_MAX) * 2.0f * (float)M_PI;
        pos.x = mundo->jugador.pos.x + sinf(angulo) * 50.0f;
        pos.z = mundo->jugador.pos.z + cosf(angulo) * 50.0f;
        intentos++;

        bool libre = true;
        for (int i = 0; i < CANT_OBSTACULOS; i++) {
            if (!mundo->obstaculos[i].destruido &&
                distancia(pos, mundo->obstaculos[i].pos) < RADIO_COLISION_MOV) {
                libre = false;
                break;
            }
        }
        if (libre) break;
    } while (intentos < 20);
    return pos;
}

static void inicializar_enemigo(tanque_t *e, const mundo_t *mundo) {
    e->pos          = posicion_enemigo(mundo);
    e->yaw          = 0.0f;
    e->yaw_torreta  = 0.0f;
    e->misil.activo = false;
    e->enfriamiento = 0.0f;
    e->vivo         = true;
    e->t_mov        = 0.0f;
    e->t_rot        = 0.0f;
    e->dir_mov      = 0;
    e->dir_rot      = 0;
}

/* ── detección de colisión entre un punto y los obstáculos/tanques ──── */

static bool hay_colision(const mundo_t *mundo, vec2_t pos,
                         const tanque_t *ignorar, float radio) {
    for (int i = 0; i < CANT_OBSTACULOS; i++) {
        if (mundo->obstaculos[i].destruido) continue;
        if (distancia(pos, mundo->obstaculos[i].pos) < radio)
            return true;
    }
    if (ignorar != &mundo->jugador &&
        mundo->jugador.vivo &&
        distancia(pos, mundo->jugador.pos) < radio)
        return true;
    if (ignorar != &mundo->enemigo &&
        mundo->enemigo.vivo &&
        distancia(pos, mundo->enemigo.pos) < radio)
        return true;
    return false;
}

static void mover_tanque(mundo_t *mundo, tanque_t *tanque, float dt) {
    if (tanque->t_rot > 0.0f) {
        float giro = (float)tanque->dir_rot * VELOCIDAD_GIRO * dt;
        tanque->yaw     = normalizar_angulo(tanque->yaw + giro);
        tanque->t_rot  -= dt;
        if (tanque->t_rot < 0.0f) tanque->t_rot = 0.0f;
    }

    if (tanque->t_mov > 0.0f) {
        float dx = sinf(tanque->yaw) * VELOCIDAD_TANQUE * dt * (float)tanque->dir_mov;
        float dz = cosf(tanque->yaw) * VELOCIDAD_TANQUE * dt * (float)tanque->dir_mov;
        vec2_t nueva = { tanque->pos.x + dx, tanque->pos.z + dz };

        if (!hay_colision(mundo, nueva, tanque, RADIO_COLISION_MOV)) {
            tanque->pos = nueva;
        }
        tanque->t_mov -= dt;
        if (tanque->t_mov < 0.0f) tanque->t_mov = 0.0f;
    }
}

static void actualizar_misil(mundo_t *mundo, misil_t *misil,
                              tanque_t *disparador, float dt) {
    if (!misil->activo) return;

    misil->tiempo_vida -= dt;
    if (misil->tiempo_vida <= 0.0f) {
        misil->activo = false;
        return;
    }

    float dx = sinf(misil->yaw) * VELOCIDAD_MISIL * dt;
    float dz = cosf(misil->yaw) * VELOCIDAD_MISIL * dt;
    misil->pos.x += dx;
    misil->pos.z += dz;

    /* colisión con obstáculos */
    for (int i = 0; i < CANT_OBSTACULOS; i++) {
        if (mundo->obstaculos[i].destruido) continue;
        if (distancia(misil->pos, mundo->obstaculos[i].pos) < RADIO_COLISION_MISIL) {
            mundo->obstaculos[i].destruido = true;
            misil->activo = false;
            return;
        }
    }

    /* colisión con tanque enemigo (si lo disparó el jugador) */
    if (disparador == &mundo->jugador && mundo->enemigo.vivo) {
        if (distancia(misil->pos, mundo->enemigo.pos) < RADIO_COLISION_MISIL) {
            misil->activo = false;
            mundo->enemigo.vivo = false;
            mundo->puntaje += 1000;
            /* animación y respawn se manejan desde aquí */
            mundo->animacion.tipo   = ANIM_ENEMIGO;
            mundo->animacion.tiempo = 0.0f;
            mundo->animacion.pos    = mundo->enemigo.pos;
            return;
        }
    }

    /* colisión con tanque jugador (si lo disparó el enemigo) */
    if (disparador == &mundo->enemigo && mundo->jugador.vivo) {
        if (distancia(misil->pos, mundo->jugador.pos) < RADIO_COLISION_MISIL) {
            misil->activo = false;
            mundo->vidas--;
            mundo->animacion.tipo     = ANIM_JUGADOR;
            mundo->animacion.tiempo   = 0.0f;
            mundo->animacion.linea_hud = 0;
            return;
        }
    }
}

#define FPS_JUEGO 24

static void actualizar_ia(mundo_t *mundo, float dt) {
    tanque_t *e = &mundo->enemigo;
    tanque_t *j = &mundo->jugador;

    if (!e->vivo || !j->vivo) return;

    /* --- torreta --- */
    float angulo_al_jugador = normalizar_angulo(
        angulo_hacia(e->pos, j->pos) - e->yaw
    );

    if (fabsf(angulo_al_jugador) < 1.0f) {
        /* jugador en rango de visión: apuntar hacia él */
        float diff = normalizar_angulo(angulo_al_jugador - e->yaw_torreta);
        float giro = 0.12f * dt;
        if (fabsf(diff) < giro)
            e->yaw_torreta = angulo_al_jugador;
        else
            e->yaw_torreta += (diff > 0 ? giro : -giro);
        e->yaw_torreta = fmaxf(-1.0f, fminf(1.0f, e->yaw_torreta));

        /* disparar si está bien apuntado */
        float dir_disparo = normalizar_angulo(e->yaw + e->yaw_torreta);
        float diff_disparo = normalizar_angulo(
            angulo_hacia(e->pos, j->pos) - dir_disparo
        );
        if (fabsf(diff_disparo) < 0.1f && e->enfriamiento <= 0.0f &&
            !e->misil.activo) {
            e->misil.activo     = true;
            e->misil.pos        = e->pos;
            e->misil.yaw        = dir_disparo;
            e->misil.tiempo_vida = VIDA_MISIL;
            e->enfriamiento     = ENFRIAMIENTO_MISIL;
        }
    } else {
        /* jugador fuera de rango: volver a reposo */
        float giro = 0.24f * dt;
        if (fabsf(e->yaw_torreta) < giro)
            e->yaw_torreta = 0.0f;
        else
            e->yaw_torreta += (e->yaw_torreta > 0 ? -giro : giro);
    }

    /* --- movimiento --- */
    if (e->t_mov <= 0.0f && e->t_rot <= 0.0f) {
        /* probabilidad 1/FPS de iniciar movimiento en este frame */
        if (rand() % FPS_JUEGO == 0) {
            if (rand() % 2 == 0) {
                /* rotación: duración aleatoria 0-3s en dirección al jugador */
                float dur = ((float)rand() / RAND_MAX) * 3.0f;
                e->t_rot   = dur;
                e->dir_rot = (angulo_al_jugador > 0) ? 1 : -1;
            } else {
                /* avance/retroceso: aleatorio entre -1 y 3 segundos */
                float dur = ((float)rand() / RAND_MAX) * 4.0f - 1.0f;
                if (dur < 0) {
                    e->t_mov  = -dur;
                    e->dir_mov = -1;
                } else {
                    e->t_mov  = dur;
                    e->dir_mov = 1;
                }
            }
        }
    }
}

#define DURACION_ANIM_ENEMIGO  2.5f   /* segundos del tiro oblicuo       */

static void actualizar_animacion(mundo_t *mundo, float dt) {
    animacion_t *anim = &mundo->animacion;

    if (anim->tipo == ANIM_NINGUNA) return;

    anim->tiempo += dt;

    if (anim->tipo == ANIM_JUGADOR) {
        /* 27 líneas del carácter #, una por frame aproximadamente */
        anim->linea_hud = (int)(anim->tiempo * FPS_JUEGO);
        if (anim->linea_hud >= 27) {
            anim->tipo = ANIM_NINGUNA;
            if (mundo->vidas < 0) {
                /* game over: no respawnear */
            } else {
                mundo->jugador.pos = (vec2_t){0.0f, 0.0f};
                mundo->jugador.yaw = 0.0f;
                mundo->jugador.misil.activo = false;
            }
        }
    } else if (anim->tipo == ANIM_ENEMIGO) {
        if (anim->tiempo >= DURACION_ANIM_ENEMIGO) {
            anim->tipo = ANIM_NINGUNA;
            /* respawnear enemigo */
            inicializar_enemigo(&mundo->enemigo, mundo);
        }
    }
}

mundo_t *mundo_crear(lista_modelos_t *modelos) {
    mundo_t *mundo = malloc(sizeof(mundo_t));
    if (mundo == NULL) return NULL;

    mundo->modelos = modelos;
    mundo->puntaje = 0;
    mundo->vidas   = VIDAS_INICIALES;

    /* jugador en el origen */
    mundo->jugador.pos          = (vec2_t){0.0f, 0.0f};
    mundo->jugador.yaw          = 0.0f;
    mundo->jugador.yaw_torreta  = 0.0f;
    mundo->jugador.misil.activo = false;
    mundo->jugador.enfriamiento = 0.0f;
    mundo->jugador.vivo         = true;
    mundo->jugador.t_mov        = 0.0f;
    mundo->jugador.t_rot        = 0.0f;
    mundo->jugador.dir_mov      = 0;
    mundo->jugador.dir_rot      = 0;

    /* obstáculos aleatorios */
    modelo_t *meshes_obs[] = {
        lista_modelos_buscar(modelos, "CUBO1"),
        lista_modelos_buscar(modelos, "CUBO2"),
        lista_modelos_buscar(modelos, "CUBO3"),
        lista_modelos_buscar(modelos, "PIRAMIDE1"),
        lista_modelos_buscar(modelos, "PIRAMIDE2"),
        lista_modelos_buscar(modelos, "PIRAMIDE3"),
    };
    const int cant_tipos = 6;

    for (int i = 0; i < CANT_OBSTACULOS; i++) {
        mundo->obstaculos[i].pos.x   = (float)(rand() % 300) - 150.0f;
        mundo->obstaculos[i].pos.z   = (float)(rand() % 300) - 150.0f;
        mundo->obstaculos[i].yaw     = ((float)rand() / RAND_MAX) * 2.0f * (float)M_PI;
        mundo->obstaculos[i].mesh    = meshes_obs[rand() % cant_tipos];
        mundo->obstaculos[i].destruido = false;
    }

    /* enemigo (necesita mundo con obstáculos ya inicializados) */
    inicializar_enemigo(&mundo->enemigo, mundo);

    /* cámara y animación */
    mundo->camara    = (camara_t){0.0f, 0.0f};
    mundo->animacion = (animacion_t){ANIM_NINGUNA, 0.0f, 0, {0.0f, 0.0f}};

    return mundo;
}

void mundo_actualizar(mundo_t *mundo, float dt) {
    if (mundo_terminado(mundo)) return;

    /* enfriar armas */
    if (mundo->jugador.enfriamiento > 0.0f) {
        mundo->jugador.enfriamiento -= dt;
        if (mundo->jugador.enfriamiento < 0.0f)
            mundo->jugador.enfriamiento = 0.0f;
    }
    if (mundo->enemigo.enfriamiento > 0.0f) {
        mundo->enemigo.enfriamiento -= dt;
        if (mundo->enemigo.enfriamiento < 0.0f)
            mundo->enemigo.enfriamiento = 0.0f;
    }

    /* mover tanques */
    if (mundo->jugador.vivo)
        mover_tanque(mundo, &mundo->jugador, dt);
    if (mundo->enemigo.vivo)
        mover_tanque(mundo, &mundo->enemigo, dt);

    /* misiles */
    actualizar_misil(mundo, &mundo->jugador.misil, &mundo->jugador, dt);
    actualizar_misil(mundo, &mundo->enemigo.misil, &mundo->enemigo, dt);

    /* IA */
    actualizar_ia(mundo, dt);

    /* animaciones */
    actualizar_animacion(mundo, dt);
}

void mundo_jugador_mover(mundo_t *mundo, int dir) {
    mundo->jugador.t_mov  = DURACION_MOVIMIENTO;
    mundo->jugador.dir_mov = dir;
}

void mundo_jugador_girar(mundo_t *mundo, int dir) {
    mundo->jugador.t_rot  = DURACION_MOVIMIENTO;
    mundo->jugador.dir_rot = dir;
}

void mundo_jugador_disparar(mundo_t *mundo) {
    tanque_t *j = &mundo->jugador;
    if (j->enfriamiento > 0.0f || j->misil.activo) return;

    j->misil.activo     = true;
    j->misil.pos        = j->pos;
    j->misil.yaw        = j->yaw;
    j->misil.tiempo_vida = VIDA_MISIL;
    j->enfriamiento     = ENFRIAMIENTO_MISIL;
}

bool mundo_terminado(const mundo_t *mundo) {
    return mundo->vidas < 0;
}

void mundo_destruir(mundo_t *mundo) {
    free(mundo);
}