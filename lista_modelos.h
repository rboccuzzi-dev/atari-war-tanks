#ifndef LISTA_MODELOS_H
#define LISTA_MODELOS_H

#include "modelo.h"
#include <stdbool.h>

typedef struct lista_modelos lista_modelos_t;

lista_modelos_t *lista_modelos_crear(void);

bool lista_modelos_cargar(lista_modelos_t *lista, const char *archivo);

modelo_t *lista_modelos_buscar(lista_modelos_t *lista, const char *nombre);

/* Libera todos los modelos y la lista misma. */
void lista_modelos_destruir(lista_modelos_t *lista);

#endif