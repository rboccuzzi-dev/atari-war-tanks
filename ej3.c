#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>


// Declarar un tipo enumerativo unidades_t para las posibles unidades de un STL.

typedef enum {
    mm, cm, m, pulgadas, pies, milis
} unidades_t;

// armo la tabla de busqueda que voy a usar mas adelante

const unidades_t tabla_unidades[] = {
    [mm] = mm,
    [cm] = cm,
    [m] = m,
    [pulgadas] = pulgadas,
    [pies] = pies,
    [milis] = milis,
};

/* Escribir una función  que lea un entero de 16 bits en formato little-endian del archivo f y lo escriba en v. 
La función debe devolver true si puede leer correctamente. */

bool leer_int16_little_endian(FILE *f, int16_t *v){
    uint8_t lectura[2];
    if(fread(lectura, 1, 2, f) == 2){
        *v = lectura[0] | lectura[1] << 8;
        return true;
    } return false;
}

// Escribir una función  similar a la anterior pero que lea un entero de 32 bits.

bool leer_int32_little_endian(FILE *f, int32_t *v){
    uint8_t lectura[4]; 
    if(fread(lectura, 1, 4, f) == 4){
        *v = lectura[0] | lectura[1] << 8 | lectura[2] << 16 | lectura[3] << 24;
        return true;
    } return false;
}

// Escribir una función que lea un flotante.

bool leer_float_little_endian(FILE *f, float *v){
    int32_t float_bits;
    if(leer_int32_little_endian(f, &float_bits)){
        *v = *(float *)&float_bits;
        return true;
    } return false;
}

/* Implementar una función que lea del archivo f un encabezado STL completo y valide cada uno de sus campos según lo ya 
descripto en la introducción de este enunciado. La función devolverá true si se pudo leer un encabezado correcto, false en 
caso contrario. */

bool leer_encabezado_stl(FILE *f){
    char tipo[3];
    int16_t reservado1; 
    int16_t reservado2; 
    int16_t version; 
    int32_t offset;

    if(fread(tipo, 1, 3, f) != 3 || tipo[0] != 'S'|| tipo[1] != 'T' || tipo[2] != 'L') return false; 
    if(!leer_int16_little_endian(f, &reservado1) || reservado1 != 0) return false;
    if(!leer_int16_little_endian(f, &reservado2) || reservado2 != 0) return false;
    if(!leer_int16_little_endian(f, &version)|| version != 3) return false;
    if(!leer_int32_little_endian(f, &offset)|| offset != 25) return false;

    return true;
}

/* Implementar una función que lea del archivo f el formato de modelos completo y devuelva las unidades y la máxima longitud 
maxlong indicado por el mismo. Debe devolver true de poder realizar la operación. */

bool leer_formato_stl(FILE *f, unidades_t *unidades, size_t *maxlong){
    int32_t tamano;
    int16_t unidad_i; 
    int16_t dimensiones; 
    int16_t coord;   
    int16_t max;

    if(!leer_int32_little_endian(f, &tamano) || tamano != 12) return false;

    if(!leer_int16_little_endian(f, &unidad_i)) return false;
    if(unidad_i < 0 || unidad_i > (sizeof(tabla_unidades) / sizeof(unidades_t))) return false; // verificacion: si el valor leido esta fuera del rango de la tabla de busqueda
    *unidades = tabla_unidades[unidad_i];

    if(!leer_int16_little_endian(f, &dimensiones) || dimensiones != 3) return false;
    if(!leer_int16_little_endian(f, &coord) || coord != 0) return false;

    if(!leer_int16_little_endian(f, &max)) return false;
    *maxlong = max;

    return true;
}

/* Implementar una función que reciba un archivo f y una máxima longitud de etiqueta maxlong y lea un modelo 3D del archivo. 
Si no hubiera ningún modelo para leer o fallara algo deberá devolver false. La función deberá escribir en la etiqueta la etiqueta 
leída (asumir que hay memoria suficiente). La función devolverá la cantidad de coordenadas leídas ncoords y un vector de flotantes
coords con todas esas coordenadas puestas una a continuación de otras (es decir, devuelve por la interfaz un vector de 
float [3 * ncoords] valores. La función devolverá la cantidad de líneas leídas ncoords y un vector de índices lineas con todos 
esos índices en secuencia (es decir devuelve por la interfaz un vector de size_t [2 * nlineas]). */

bool leer_modelo_3d(FILE *f, size_t maxlong, char *etiqueta, size_t *ncoords, float **coords, size_t *nlineas, size_t **lineas){
    if(fread(etiqueta, 1, maxlong, f) != maxlong) return false;
    etiqueta[maxlong - 1] = '\0';

    int32_t bcoords;
    if(!leer_int32_little_endian(f, &bcoords)) return false;
    *ncoords = bcoords;

    float *vflotantes = malloc(3 * (*ncoords) * sizeof(float));
    if (vflotantes == NULL) return false; 
    for (size_t i = 0; i < (3 * (*ncoords)); i++) {
        if (!leer_float_little_endian(f, &vflotantes[i])) {
            free(vflotantes); 
            return false;
        }
    }
    *coords = vflotantes;

    int32_t bnlineas;
    if(!leer_int32_little_endian(f, &bnlineas)){
        free(vflotantes);
        return false;
    }
    *nlineas = bnlineas;

    int32_t indice;
    size_t *vindices = malloc(2 * (*nlineas) * sizeof(size_t));
    if(vindices == NULL){
        free(vflotantes);
        return false;
    }
    for (size_t i = 0; i < 2 * (*nlineas); i++) {
        if (!leer_int32_little_endian(f, &indice)) { 
            free(vflotantes);
            free(vindices);
            return false;
        }
        vindices[i] = indice; 
    }
    *lineas = vindices;
    return true;
}






