#ifndef MANDELBROT_H
#define MANDELBROT_H

#define RADIO_DE_ESCAPE_CUADRADO 4.0

typedef struct
{
    double x_min;
    double x_max;
    double y_min;
    double y_max;
} Region;

static inline int mandelbrot_iteraciones(double cr, double ci, int max_iter)
{
    double zr = 0.0;
    double zi = 0.0;
    int iter = 0;

    while (zr * zr + zi * zi <= RADIO_DE_ESCAPE_CUADRADO
           && iter < max_iter)
    {
        double nuevo_zr = zr * zr - zi * zi + cr;
        double nuevo_zi = 2.0 * zr * zi + ci;

        zr = nuevo_zr;
        zi = nuevo_zi;
        iter++;
    }

    return iter;
}

#endif