#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>

#include "config.h"
#include "mandelbrot.h"
#include "colormap.h"

int main(void)
{
    enum {
        ancho = DEFAULT_ANCHO,
        alto = DEFAULT_ALTO
    };

    static int imagen[alto][ancho];
    int max_iter = DEFAULT_MAX_ITER;

    double a_min = DEFAULT_X_MIN;
    double a_max = DEFAULT_X_MAX;
    double b_min = DEFAULT_Y_MIN;
    double b_max = DEFAULT_Y_MAX;

    double paso_a = (a_max - a_min) / (ancho - 1);
    double paso_b = (b_max - b_min) / (alto - 1);

    long long trabajo = 0;
    struct timespec inicio, fin;

    if (clock_gettime(CLOCK_MONOTONIC, &inicio) != 0)
    {
        perror("No se pudo iniciar la medicion");
        return 1;
    }

    // Un único proceso calcula todas las filas.
    for (int fila = 0; fila < alto; fila++)
    {
        double b = b_max - fila * paso_b;

        for (int col = 0; col < ancho; col++)
        {
            double a = a_min + col * paso_a;
            int resultado = mandelbrot_iteraciones(a, b, max_iter);

            imagen[fila][col] = resultado;
            trabajo += resultado;
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &fin) != 0)
    {
        perror("No se pudo finalizar la medicion");
        return 1;
    }

    double tiempo_total =
        (double)(fin.tv_sec - inicio.tv_sec) +
        (double)(fin.tv_nsec - inicio.tv_nsec) / 1000000000.0;

    // Impresión y escritura fuera de la medición.
    printf("Secuencial: %lld iteraciones\n", trabajo);
    printf("Tiempo secuencial: %.6f segundos\n", tiempo_total);

    int error_archivo = 0;
    if (GENERAR_SALIDAS)
    {
        if (escribir_iteraciones("resultados/secuencial_iter.txt", ancho, alto, &imagen[0][0]) != 0)
        {
            fprintf(stderr, "Error al guardar las iteraciones\n");
            error_archivo = 1;
        }

        colormap_func_t paleta = select_colormap("hot");

        FILE *archivo = fopen("resultados/mandelbrot_secuencial.ppm", "w");

        if (archivo == NULL)
        {
            perror("No se pudo crear la imagen");
            error_archivo = 1;
        }
        else
        {
            fprintf(archivo, "P3\n%d %d\n255\n", ancho, alto);

            for (int fila = 0; fila < alto; fila++)
            {
                for (int col = 0; col < ancho; col++)
                {
                    int iter = imagen[fila][col];
                    int inside_color = (iter == max_iter);

                    unsigned char r, g, b;
                    paleta(iter, max_iter, &r, &g, &b, inside_color);

                    fprintf(archivo, "%u %u %u ",
                            (unsigned int)r,
                            (unsigned int)g,
                            (unsigned int)b);
                }

                fprintf(archivo, "\n");
            }

            if (fclose(archivo) != 0)
            {
                perror("No se pudo terminar de guardar la imagen");
                error_archivo = 1;
            }
        }
    }

    return error_archivo;
}