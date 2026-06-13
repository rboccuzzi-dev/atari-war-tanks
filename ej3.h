#ifndef EJ3_H
#define EJ3_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

typedef enum {
    mm, cm, m, pulgadas, pies, milis
} unidades_t;

bool leer_int16_little_endian(FILE *f, int16_t *v);
bool leer_int32_little_endian(FILE *f, int32_t *v);
bool leer_float_little_endian(FILE *f, float *v);

bool leer_encabezado_stl(FILE *f);

bool leer_formato_stl(
    FILE *f,
    unidades_t *unidades,
    size_t *maxlong
);

bool leer_modelo_3d(
    FILE *f,
    size_t maxlong,
    char *etiqueta,
    size_t *ncoords,
    float **coords,
    size_t *nlineas,
    size_t **lineas
);

#endif