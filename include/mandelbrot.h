#ifndef MANDELBROT_H
#define MANDELBROT_H

//valores por defecto de la región del plano complejo
#define DEFAULT_X_MIN -2.5
#define DEFAULT_X_MAX  1.5
#define DEFAULT_Y_MIN -1.5
#define DEFAULT_Y_MAX  1.5

// límite de divergencia, si |z|^2 supera el valor, la sucesión escapa
#define RADIO_DE_ESCAPE_CUADRADO 4.0

// estructura que representa una región del plano complejo
typedef struct{
    double x_min;
    double x_max;
    double y_min;
    double y_max;
} Region;



static inline int mandelbrot_iteraciones(double cr, double ci, int max_iter) {
    double zr = 0.0; //parte real de z
    double zi = 0.0; //parte real de z
    double zr2 = 0.0; //zr al cuadrado
    double zi2 = 0.0; //zi al cuadrado

    int i;
    for (i=0; i< max_iter; ++i){
        zi =  2.0 * zr * zi  + ci;
        zr = zr2 - zi2 + cr;
        zr2 = zr * zr;
        zi2 = zi * zi;

        if(zr2 + zi2 > RADIO_DE_ESCAPE_CUADRADO) {
            // si llega a ser mayor al radio de escape devuelve la iteracion a la que llegó
            return i; 
        }
    }
    return max_iter;
}