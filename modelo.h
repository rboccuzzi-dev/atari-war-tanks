#ifndef MODELO_H
#define MODELO_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    char    nombre[32];

    size_t  ncoords;
    float  *coords;     /* vector de 3*ncoords floats: x0,y0,z0, x1,y1,z1, ... */

    size_t  nlineas;
    size_t *lineas;     /* vector de 2*nlineas índices: a0,b0, a1,b1, ...       */
} modelo_t;

void modelo_destruir(modelo_t *m);

#endif