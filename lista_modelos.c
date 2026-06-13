#include "lista_modelos.h"
#include "ej3.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct lista_modelos {
    modelo_t *items;   /* array de modelos */
    size_t    cantidad;
    size_t    capacidad;
};

lista_modelos_t *lista_modelos_crear(void) {
    lista_modelos_t *lista = malloc(sizeof(lista_modelos_t));
    if (lista == NULL) return NULL;

    lista->capacidad = 16;
    lista->cantidad  = 0;
    lista->items     = malloc(lista->capacidad * sizeof(modelo_t));
    if (lista->items == NULL) {
        free(lista);
        return NULL;
    }
    return lista;
}

/* Agrega una copia del modelo al final de la lista. */
static bool agregar(lista_modelos_t *lista, modelo_t *m) {
    if (lista->cantidad == lista->capacidad) {
        size_t nueva_cap = lista->capacidad * 2;
        modelo_t *nuevo  = realloc(lista->items, nueva_cap * sizeof(modelo_t));
        if (nuevo == NULL) return false;
        lista->items    = nuevo;
        lista->capacidad = nueva_cap;
    }
    lista->items[lista->cantidad++] = *m;
    return true;
}

bool lista_modelos_cargar(lista_modelos_t *lista, const char *archivo) {
    FILE *f = fopen(archivo, "rb");
    if (f == NULL) {
        perror("lista_modelos_cargar: fopen");
        return false;
    }

    if (!leer_encabezado_stl(f)) {
        fprintf(stderr, "lista_modelos_cargar: encabezado STL inválido\n");
        fclose(f);
        return false;
    }

    unidades_t unidades;
    size_t     maxlong;
    if (!leer_formato_stl(f, &unidades, &maxlong)) {
        fprintf(stderr, "lista_modelos_cargar: formato STL inválido\n");
        fclose(f);
        return false;
    }

    char *etiqueta = malloc(maxlong);
    if (etiqueta == NULL) {
        fclose(f);
        return false;
    }

    modelo_t m;
    bool al_menos_uno = false;

    while (leer_modelo_3d(f, maxlong, etiqueta,
                          &m.ncoords, &m.coords,
                          &m.nlineas, &m.lineas)) {
        strncpy(m.nombre, etiqueta, sizeof(m.nombre) - 1);
        m.nombre[sizeof(m.nombre) - 1] = '\0';

        if (!agregar(lista, &m)) {
            /* Si no podemos agregar, liberamos este modelo y paramos */
            modelo_destruir(&m);
            break;
        }
        al_menos_uno = true;
    }

    free(etiqueta);
    fclose(f);
    return al_menos_uno;
}

modelo_t *lista_modelos_buscar(lista_modelos_t *lista, const char *nombre) {
    for (size_t i = 0; i < lista->cantidad; i++) {
        if (strcmp(lista->items[i].nombre, nombre) == 0)
            return &lista->items[i];
    }
    return NULL;
}

void lista_modelos_destruir(lista_modelos_t *lista) {
    if (lista == NULL) return;
    for (size_t i = 0; i < lista->cantidad; i++) {
        modelo_destruir(&lista->items[i]);
    }
    free(lista->items);
    free(lista);
}