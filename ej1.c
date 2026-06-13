#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

// inicializar matriz

void inicializar(size_t n, float m[n][n]){

    for (size_t fila = 0; fila < n; fila++){
        for (size_t col = 0; col < n; col++){
            if (fila == col){
                m[fila][col] = 1;
            }
            else{
                m[fila][col] = 0;
            }
        } 
    }
    m[n - 1][n - 2] = -1;
    m[n - 1][n - 1] = 0;
}

// incrementar vector

void incrementar_vector(size_t n, float v[n], const float inc[n]){
    for (size_t col = 0; col < n; col++){
        v[col] += inc[col];
    }
}

// multiplicación matriz por vector tal que r = A * v

void multiplicar(size_t n, size_t m, float r[n], float a[n][m], const float v[m]){

    for (size_t fila = 0; fila < n; fila++){ 
        r[fila] = 0;
        for (size_t col = 0; col < m; col++){
            r[fila] += a[fila][col] * v[col];
        }
    }
}

// implementar una función que opere tal que r = Mb * v

void transformar_b(float r[3], const float v[3], double ab){

    float mb[3][3] = {
        {cos(ab), 0, sin(ab)},
        {0, 1, 0},
        {-sin(ab), 0, cos(ab)},
    };

    multiplicar(3, 3, r, mb, v);
}

// implementar una función que opere tal que r = Mc * v

void transformar_c(float r[3], const float v[3], double ac){

    float mc[3][3] = {
        {cos(ac), -sin(ac), 0},
        {sin(ac), cos(ac), 0},
        {0, 0, 1},
    };

    multiplicar(3, 3, r, mc, v);
}

// no tengo forma de explicar lo que se supone que hace esto con palabras

void aplicar_vector(float r[3], const float p[3])
{
    // esto lo deje como un ph[4] fijo (en vez de usar una variable) por como esta escrita la funcion en el enunciado (a)
    float ph[4]; 
    // le sumo un 1 al final del vector p -> ph
    for(int i = 0; i < 3; i++){
        ph[i] = p[i];
    }
    ph[3] = 1;
   
    // inicializo m4

    float matriz[4][4];
    inicializar(4, matriz);

    // multiplico m4 con ph -> m

    float m[4];
    multiplicar(4, 4, m, matriz, ph);

    // escribo un vector de 3 elementos -> r = (x/w, y/w, z/w)
    for(int i = 0; i < 3; i++){
        r[i] = m[i]/m[3];
    }
}

// leer vector

bool leer_vector(float v[3]){

    // inicializo en cero el vector final donde van a estar los numeros como flotantes

    for (int posicion = 0; posicion < 3; posicion++){
        v[posicion] = 0;
    }

    char cadena_usuario[100]; // la cadena completa que ingresa el usuario
    char numero_individual[20]; // los numeros puestos individualmente (pre-atof)
    int numeros_totales = 0;
    int cant_caracteres = 0; // cantidad de caracteres que tiene cada numero individualmente

    if((fgets(cadena_usuario, 100, stdin)) == NULL){
        return false; // no se puede leer
    }

    for(int indice_usuario = 0; cadena_usuario[indice_usuario] != '\0' && numeros_totales < 3; indice_usuario++){

        char caracter = cadena_usuario[indice_usuario]; // voy guardando en caracter lo que ingreso el usuario

        if(caracter == ' ' ||  caracter == '\n' ||  caracter == ','){ // llego al final del numero o de la cadena

            numero_individual[cant_caracteres] = '\0'; // indica el final del numero
            v[numeros_totales] = atof(numero_individual); 
            numeros_totales ++;
            cant_caracteres = 0; 

        }
        else {
            numero_individual[cant_caracteres] = caracter; 
            cant_caracteres ++;
        }
    }

    if(numeros_totales == 3){
        return true;
    }
    return false;
}
