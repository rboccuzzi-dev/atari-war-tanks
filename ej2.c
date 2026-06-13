#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

// modelar matriz

typedef struct {
    float *m; 
    size_t filas, columnas;
} matriz_t; 

// crear y destruir matriz

matriz_t *_matriz_crear(size_t n, size_t m){
    matriz_t *matriz = malloc(sizeof(matriz_t)); // creo un puntero de tipo struct matriz_t de nombre matriz que inicializa una posicion de memoria de tamaño matriz_t
    
    if(matriz == NULL){ 
        return NULL; 
    }

    matriz->m = malloc(n * m * sizeof(matriz));  // le guardo la memoria que va a ocupar mi matriz 
    
    if(matriz->m == NULL){ 
        free(matriz);
        return NULL;
    }

    matriz->filas = n; 
    matriz->columnas = m;  

    return matriz;  
}

void matriz_destruir(matriz_t *matriz) {
    if (matriz != NULL) {  
        free(matriz->m); 
        free(matriz);    
    }
}

// acceso a los datos

// implementar una función que devuelva el número de filas de la matriz

size_t matriz_filas(const matriz_t *matriz){
    return matriz->filas;
}

// implementar una función que devuelva el número de columnas de la matriz.

size_t matriz_columnas(const matriz_t *matriz){
    return matriz->columnas;
}

// implementar una función que devuelva por la interfaz la cantidad de filas y de columnas de la matriz.

void matriz_dimensiones(const matriz_t *matriz, size_t *filas, size_t *columnas){
    *filas = matriz_filas(matriz);
    *columnas = matriz_columnas(matriz);
}

// implementar una función que devuelva el valor almacenado en la posición fila, columna de la matriz.

float matriz_obtener(const matriz_t *matriz, size_t fila, size_t columna){
    return matriz->m[(fila * matriz->columnas) + columna];
}

// implementar una función que establezca el valor en la posición fila, columna de la matriz.

void matriz_establecer(matriz_t *matriz, size_t fila, size_t columna, float valor){
    matriz->m[(fila * matriz->columnas) + columna] = valor;
}

// creacion de matrices

// hago una funcion auxiliar que cree la identidad

matriz_t *crear_identidad(size_t n) {
    matriz_t *identidad = _matriz_crear(n, n);
    
    if (identidad == NULL) return NULL;
    
    for(size_t i = 0; i < n; i++){
        for(size_t j = 0; j < n; j++){
            if(i == j){
                matriz_establecer(identidad, i, j, 1.0); 
            } else {
                matriz_establecer(identidad, i, j, 0.0);
            }
        }
    }
    return identidad;
}

matriz_t *matriz_crear_mn(size_t n){
    matriz_t *matriz_mn = crear_identidad(n);
    if(matriz_mn == NULL) return NULL;

    matriz_establecer(matriz_mn, n - 1, n - 1, 0);
    matriz_establecer(matriz_mn, n - 1, n - 2, -1);

    return matriz_mn;
}

matriz_t *matriz_crear_mc(double ac){
    matriz_t *matriz_mc = crear_identidad(4);
    if(matriz_mc == NULL) return NULL;

    float coseno = cos(ac);
    float seno = sin(ac);

    matriz_establecer(matriz_mc, 0, 0, coseno);
    matriz_establecer(matriz_mc, 0, 1, -seno);
    matriz_establecer(matriz_mc, 1, 0, seno);
    matriz_establecer(matriz_mc, 1, 1, coseno);

    return matriz_mc;
}

matriz_t *matriz_crear_mb(double ab){
    matriz_t *matriz_mb = crear_identidad(4);
    if(matriz_mb == NULL) return NULL;

    float coseno = cos(ab);
    float seno = sin(ab);

    matriz_establecer(matriz_mb, 0, 0, coseno);
    matriz_establecer(matriz_mb, 0, 2, seno);
    matriz_establecer(matriz_mb, 2, 0, -seno);
    matriz_establecer(matriz_mb, 2, 2, coseno);

    return matriz_mb;
}

matriz_t *matriz_crear_mt(const float vector[3]){
    matriz_t *matriz_mt = crear_identidad(4);
    if (matriz_mt == NULL) return NULL;

    matriz_establecer(matriz_mt, 0, 3, vector[0]);
    matriz_establecer(matriz_mt, 1, 3, vector[1]);
    matriz_establecer(matriz_mt, 2, 3, vector[2]);

    return matriz_mt;
}

// multiplicacion de matrices

matriz_t *matriz_multiplicar(const matriz_t *a, const matriz_t *b){
    if(a->columnas != b->filas) return NULL; // incompatibles
    
    matriz_t *resultado_multiplicar = _matriz_crear(a->filas, b->columnas);
    if(resultado_multiplicar == NULL) return NULL;

    for(size_t i = 0; i < a->filas; i++){
        for(size_t j = 0; j < b->columnas; j++){
            float suma = 0;
            for(size_t k = 0; k < a->columnas; k++){
                suma += matriz_obtener(a, i, k) * matriz_obtener(b, k, j);
                matriz_establecer(resultado_multiplicar, i, j, suma);
            }
        }
    }
    return resultado_multiplicar;
}

// aplicar

matriz_t *matriz_aplicar(const matriz_t *matriz, const matriz_t *ps) {
    if (matriz->filas != 4 || matriz->columnas != 4 || ps->columnas != 3) return NULL;

    size_t n = ps->filas;
    matriz_t *resultado = _matriz_crear(n, 3);
    if (resultado == NULL) return NULL;

    for (size_t i = 0; i < n; i++) {
        float x = matriz_obtener(ps, i, 0);
        float y = matriz_obtener(ps, i, 1);
        float z = matriz_obtener(ps, i, 2);
        float w = 1.0;

        float aux[4];
        for (size_t j = 0; j < 4; j++) {
            aux[j] = matriz_obtener(matriz, j, 0) * x
                    + matriz_obtener(matriz, j, 1) * y
                    + matriz_obtener(matriz, j, 2) * z
                    + matriz_obtener(matriz, j, 3) * w;
        }

        float wg = aux[3];
        matriz_establecer(resultado, i, 0, aux[0] / wg);
        matriz_establecer(resultado, i, 1, aux[1] / wg);
        matriz_establecer(resultado,i,2,w);
    }
    return resultado;
}

bool matriz_agregar_fila(matriz_t *matriz, float fila[]) {
    if (matriz == NULL || fila == NULL) return false;
 
    size_t fila_nueva = matriz->filas + 1;
    size_t total_elementos = fila_nueva * matriz->columnas;
 
    float *aux = realloc(matriz->m, total_elementos * sizeof(float));
    if (aux == NULL) return false;
 
    matriz->m = aux;
    matriz->filas = fila_nueva;
 
    size_t inicio = (fila_nueva - 1) * matriz->columnas;
    for (size_t j = 0; j < matriz->columnas; j++) {
        matriz->m[inicio + j] = fila[j];
    }
    return true;
}
