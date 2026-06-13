#ifndef MUNDO_H
#define MUNDO_H

#include "modelo.h"
#include "lista_modelos.h"
#include <stdbool.h>
#include <stddef.h>


#define CANT_OBSTACULOS     50
#define VIDAS_INICIALES      3
#define RADIO_COLISION_MOV   5.0f   /* metros: colisión para movimiento  */
#define RADIO_COLISION_MISIL 3.0f   /* metros: colisión para impacto     */
#define VELOCIDAD_TANQUE     7.0f   /* m/s                               */
#define VELOCIDAD_GIRO       0.8f  /* rad/s                             */
#define VELOCIDAD_MISIL     24.0f   /* m/s                               */
#define VIDA_MISIL           2.0f   /* segundos hasta desaparecer        */
#define ENFRIAMIENTO_MISIL   2.0f   /* segundos entre disparos           */
#define DURACION_MOVIMIENTO  0.5f   /* segundos que dura un comando      */
#define ALTURA_CAMARA        3.0f   /* metros de altura de la cámara     */


typedef struct {
    float x;
    float z;
} vec2_t;

typedef struct {
    vec2_t pos;
    float  yaw;          /* dirección de vuelo                          */
    float  tiempo_vida;  /* segundos restantes antes de desaparecer     */
    bool   activo;
} misil_t;

typedef struct {
    vec2_t   pos;
    float    yaw;
    modelo_t *mesh;
    bool     destruido;
} obstaculo_t;

typedef struct {
    vec2_t pos;
    float  yaw;           /* dirección del cuerpo                       */
    float  yaw_torreta;   /* relativo al cuerpo (solo enemigo lo mueve) */

    misil_t misil;
    float   enfriamiento; /* segundos restantes hasta poder disparar    */

    bool    vivo;

    /* movimiento en curso (se consume en actualizar) */
    float   t_mov;        /* tiempo restante del movimiento actual      */
    float   t_rot;        /* tiempo restante de la rotación actual      */
    int     dir_mov;      /* +1 adelante, -1 atrás, 0 quieto            */
    int     dir_rot;      /* +1 derecha, -1 izquierda, 0 quieto         */
} tanque_t;


typedef struct {
    float angx;  /* inclinación vertical (vibración al moverse)         */
    float angz;  /* ladeado (vibración al girar)                        */
} camara_t;


typedef enum {
    ANIM_NINGUNA,
    ANIM_JUGADOR,   /* pantalla rompiéndose (#) línea por línea          */
    ANIM_ENEMIGO    /* tiro oblicuo con piezas del tanque                */
} tipo_animacion_t;

typedef struct {
    tipo_animacion_t tipo;
    float            tiempo;     /* segundos transcurridos               */
    int              linea_hud;  /* para ANIM_JUGADOR: línea del # actual */
    /* posición donde ocurrió (para ANIM_ENEMIGO) */
    vec2_t           pos;
} animacion_t;

typedef struct {
    tanque_t      jugador;
    tanque_t      enemigo;

    obstaculo_t   obstaculos[CANT_OBSTACULOS];

    camara_t      camara;
    animacion_t   animacion;

    int           puntaje;
    int           vidas;       /* vidas restantes sin contar la actual   */

    lista_modelos_t *modelos;  /* todos los meshes cargados del STL      */
} mundo_t;


/*
 * mundo_crear: inicializa el estado completo del juego.
 * Recibe la lista de modelos ya cargada.
 * Devuelve NULL si falla.
 */
mundo_t *mundo_crear(lista_modelos_t *modelos);

/*
 * mundo_actualizar: avanza la simulación dt segundos.
 * Mueve tanques, misiles, corre la IA enemiga, detecta colisiones.
 */
void mundo_actualizar(mundo_t *mundo, float dt);

/*
 * mundo_jugador_mover: registra un comando de movimiento del jugador.
 * dir: +1 adelante, -1 atrás.
 */
void mundo_jugador_mover(mundo_t *mundo, int dir);

/*
 * mundo_jugador_girar: registra un comando de giro del jugador.
 * dir: +1 derecha, -1 izquierda.
 */
void mundo_jugador_girar(mundo_t *mundo, int dir);

/*
 * mundo_jugador_disparar: intenta disparar el misil del jugador.
 * No hace nada si el enfriamiento no terminó.
 */
void mundo_jugador_disparar(mundo_t *mundo);

/*
 * mundo_terminado: devuelve true si el jugador se quedó sin vidas.
 */
bool mundo_terminado(const mundo_t *mundo);

/*
 * mundo_destruir: libera la memoria del mundo.
 * No libera la lista de modelos (es responsabilidad del llamador).
 */
void mundo_destruir(mundo_t *mundo);

#endif