#include <stdio.h>
#include <mpi.h>

#include "config.h"
#include "mandelbrot.h"
#include "colormap.h"

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank;
    int procesos;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &procesos);

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

	// Barrier antes de comenzar la medición
    MPI_Barrier(MPI_COMM_WORLD);
    double inicio = MPI_Wtime();

    // Cada proceso calcula solo sus filas
    for (int fila = rank; fila < alto; fila += procesos)
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

    // Juntamos las filas en P0
    if (rank != 0)
    {
        for (int fila = rank; fila < alto; fila += procesos)
        {
            MPI_Send(imagen[fila], ancho, MPI_INT,
                     0, fila, MPI_COMM_WORLD);
        }
    }
    else
    {
        for (int fila = 0; fila < alto; fila++)
        {
            int origen = fila % procesos;

            if (origen != 0)
            {
                MPI_Recv(imagen[fila], ancho, MPI_INT,
                         origen, fila, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);
            }
        }
    }

    // Cada proceso mide su duración con su propio reloj
    double tiempo_local = MPI_Wtime() - inicio;
    double tiempo_total = 0.0;

    MPI_Reduce(&tiempo_local, &tiempo_total,
               1, MPI_DOUBLE, MPI_MAX,
               0, MPI_COMM_WORLD);

    // No medimos el tiempo de impresión ni la escritura
    printf("P%d: %lld iteraciones\n", rank, trabajo);

    int error_archivo = 0;

    if (rank == 0)
    {
        printf("Tiempo con %d procesos: %.6f segundos\n", procesos, tiempo_total);

        if (escribir_iteraciones("resultados/ciclico_iter.txt",
                                ancho, alto, &imagen[0][0]) != 0)
        {
            error_archivo = 1;
        }

        colormap_func_t paleta = select_colormap("hot");

        FILE *archivo =
            fopen("resultados/mandelbrot_ciclico.ppm", "w");

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

    MPI_Finalize();
    return error_archivo;
}