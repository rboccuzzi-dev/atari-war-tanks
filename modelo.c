#include "modelo.h"
#include <stdlib.h>

void modelo_destruir(modelo_t *m) {
    if (m == NULL) return;
    free(m->coords);
    free(m->lineas);
    m->coords = NULL;
    m->lineas = NULL;
    m->ncoords = 0;
    m->nlineas = 0;
}