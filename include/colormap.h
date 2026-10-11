/* Convierte un número de iteraciones en un color RGB.*/
#ifndef COLORMAP_H
#define COLORMAP_H

#include <math.h>

/* Paleta "hot": negro -> rojo -> amarillo -> blanco.
 * inside_color indica si el punto pertenece al conjunto (max_iter).
 */
static inline void hot_colormap(int iter, int max_iter,
                                unsigned char *r, unsigned char *g,
                                unsigned char *b, int inside_color) {
    if (inside_color) {
        *r = 0; *g = 0; *b = 0;
        return;
    }

    double t = (double)iter / (double)max_iter;
    *r = (unsigned char)(255.0 * fmin(1.0, 3.0 * t));
    *g = (unsigned char)(255.0 * fmax(0.0, fmin(1.0, 3.0 * t - 1.0)));
    *b = (unsigned char)(255.0 * fmax(0.0, fmin(1.0, 3.0 * t - 2.0)));
}

/*Paleta "magma": tonos púrpuras, rojos y amarillos.*/
static inline void magma_colormap(int iter, int max_iter,
                                  unsigned char *r, unsigned char *g,
                                  unsigned char *b, int inside_color) {
    if (inside_color) {
        *r = 0; *g = 0; *b = 0;
        return;
    }

    double t = (double)iter / (double)max_iter;
    *r = (unsigned char)(255.0 * fmin(1.0, 2.5 * t));
    *g = (unsigned char)(255.0 * fmax(0.0, fmin(1.0, 2.5 * (t - 0.4))));
    *b = (unsigned char)(255.0 * fmax(0.0, fmin(1.0, 2.5 * (t - 0.7))));
}

/*Paleta "ocean": tonos azules y verdes.*/
static inline void ocean_colormap(int iter, int max_iter,
                                  unsigned char *r, unsigned char *g,
                                  unsigned char *b, int inside_color) {
    if (inside_color) {
        *r = 0; *g = 0; *b = 0;
        return;
    }

    double t = (double)iter / (double)max_iter;
    *r = (unsigned char)(255.0 * fmax(0.0, fmin(1.0, 2.0 * (t - 0.5))));
    *g = (unsigned char)(255.0 * fmin(1.0, 2.0 * t));
    *b = (unsigned char)(255.0 * fmin(1.0, 1.5 * t + 0.2));
}

typedef void (*colormap_func_t)(int, int, unsigned char *, unsigned char *,
                                unsigned char *, int);

/*Permite seleccionar la paleta por nombre.*/
static colormap_func_t select_colormap(const char *name) {
    if (name == NULL) return hot_colormap;
    if (name[0] == 'm' || name[0] == 'M') return magma_colormap;
    if (name[0] == 'o' || name[0] == 'O') return ocean_colormap;
    return hot_colormap;
}

#endif 
