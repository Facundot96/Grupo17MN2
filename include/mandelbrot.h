#ifndef MANDELBROT_H
#define MANDELBROT_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//valores por defecto de la región del plano complejo
#define DEFAULT_X_MIN -2.5
#define DEFAULT_X_MAX  1.5
#define DEFAULT_Y_MIN -1.5
#define DEFAULT_Y_MAX  1.5

//valores por defetco de la imagen y las iteraciones
#define DEFAULT_WIDTH        1024
#define DEFAULT_HEIGHT       1024
#define DEFAULT_MAX_ITER     1000
#define DEFAULT_FILAS_BLOQUE    8

// límite de divergencia, si |z|^2 supera el valor, la sucesión escapa
#define RADIO_DE_ESCAPE_CUADRADO 4.0

// estructura que representa una región del plano complejo
typedef struct{
    double x_min;
    double x_max;
    double y_min;
    double y_max;
} Region;

//configuración completa de la ejecución
typedef struct {
    int width;
    int height;
    int max_iter;
    int filas_bloque;  // filas por tarea (maestro-trabajador) o por bloque (estático)
    Region region;
} Config;

//valores por defecto de la configuración
static inline Config config_default(void) {
    Config cfg;
    cfg.width           = DEFAULT_WIDTH;
    cfg.height          = DEFAULT_HEIGHT;
    cfg.max_iter        = DEFAULT_MAX_ITER;
    cfg.filas_bloque    = DEFAULT_FILAS_BLOQUE;
    cfg.region.x_min    = DEFAULT_X_MIN;
    cfg.region.x_max    = DEFAULT_X_MAX;
    cfg.region.y_min    = DEFAULT_Y_MIN;
    cfg.region.y_max    = DEFAULT_Y_MAX;
    return cfg;
}


// z_0 = 0
// z_{n+1} = z_n^2 + c
static inline int mandelbrot_iteraciones(double cr, double ci, int max_iter) {
    double zr = 0.0; //parte real de z
    double zi = 0.0; //parte real de z
    double zr2 = 0.0; //zr al cuadrado
    double zi2 = 0.0; //zi al cuadrado

    int i;
    for (i=0; i < max_iter; ++i){
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

/*
 * Escribe una imagen en formato PGM (escala de grises) a partir de un
 * buffer de iteraciones. Los puntos que pertenecen al conjunto se pintan
 * de negro y los demás en escala de grises proporcional a las iteraciones.
 */
static inline int escribir_pgm(const char *nombre, const Config *cfg,
                               const int *imagen) {
    FILE *fp = fopen(nombre, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Error: no se pudo crear %s\n", nombre);
        return -1;
    }

    fprintf(fp, "P5\n%d %d\n255\n", cfg->width, cfg->height);
    for (int j = 0; j < cfg->height; ++j) {
        for (int i = 0; i < cfg->width; ++i) {
            int iter = imagen[j * cfg->width + i];
            unsigned char c;
            if (iter >= cfg->max_iter) {
                c = 0;
            } else {
                c = (unsigned char)(255.0 * iter / cfg->max_iter);
            }
            fputc(c, fp);
        }
    }
    fclose(fp);
    return 0;
}

/*
 * Escribe una imagen en formato PPM (color) usando la paleta indicada.
 */
static inline int escribir_ppm(const char *nombre, const Config *cfg,
                               const int *imagen,
                               void (*colormap)(int, int, unsigned char *,
                                                unsigned char *, unsigned char *, int)) {
    FILE *fp = fopen(nombre, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Error: no se pudo crear %s\n", nombre);
        return -1;
    }

    fprintf(fp, "P6\n%d %d\n255\n", cfg->width, cfg->height);
    for (int j = 0; j < cfg->height; ++j) {
        for (int i = 0; i < cfg->width; ++i) {
            int iter = imagen[j * cfg->width + i];
            unsigned char r, g, b;
            colormap(iter, cfg->max_iter, &r, &g, &b, iter >= cfg->max_iter);
            fputc(r, fp);
            fputc(g, fp);
            fputc(b, fp);
        }
    }
    fclose(fp);
    return 0;
}

#endif