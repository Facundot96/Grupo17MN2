#ifndef CONFIG_H
#define CONFIG_H

#define DEFAULT_ANCHO 2000
#define DEFAULT_ALTO  2000
#define DEFAULT_MAX_ITER 1000

// Cambiar este número para seleccionar R1, R2, R3 o R4.
#define REGION_ACTIVA 1

#if REGION_ACTIVA == 1

#define DEFAULT_X_MIN -2.00
#define DEFAULT_X_MAX  0.47
#define DEFAULT_Y_MIN -1.12
#define DEFAULT_Y_MAX  1.12

#elif REGION_ACTIVA == 2

#define DEFAULT_X_MIN  0.30
#define DEFAULT_X_MAX  0.90
#define DEFAULT_Y_MIN -0.30
#define DEFAULT_Y_MAX  0.30

#elif REGION_ACTIVA == 3

#define DEFAULT_X_MIN -0.15
#define DEFAULT_X_MAX  0.15
#define DEFAULT_Y_MIN -0.15
#define DEFAULT_Y_MAX  0.15

#elif REGION_ACTIVA == 4

#define DEFAULT_X_MIN -0.78
#define DEFAULT_X_MAX -0.72
#define DEFAULT_Y_MIN  0.07
#define DEFAULT_Y_MAX  0.13

#else
#error "REGION_ACTIVA debe estar entre 1 y 4"
#endif

#endif