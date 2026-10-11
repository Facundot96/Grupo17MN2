#include <stdio.h>
#include <mpi.h>
#include "include/colormap.h"

#define X_MIN -2.5
#define X_MAX  1.0
#define Y_MIN -1.25
#define Y_MAX  1.25

int mandelbrot(double a, double b, int max_iter)
{
    double x = 0.0;
    double y = 0.0;
    int iter = 0;

    while (x * x + y * y <= 4.0 && iter < max_iter)
    {
        double new_x = x * x - y * y + a;
        double new_y = 2 * x * y + b;

        x = new_x;
        y = new_y;
        iter++;
    }

    return iter;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank;
    int procesos;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &procesos);

    enum { ancho = 2800, alto = 2000 };
    static int imagen[alto][ancho];

    int max_iter = 100;

	double a_min = X_MIN;
	double a_max = X_MAX;
	double b_min = Y_MIN;
	double b_max = Y_MAX;

    double paso_a = (a_max - a_min) / (ancho - 1);
    double paso_b = (b_max - b_min) / (alto - 1);

	long long trabajo = 0;

	int inicio_fila = rank * alto / procesos;
	int fin_fila = (rank + 1) * alto / procesos;

    // Barrier antes de comenzar la medición
    MPI_Barrier(MPI_COMM_WORLD);
    double inicio = MPI_Wtime();

    // Cada proceso calcula solo sus filas
    for (int fila = inicio_fila; fila < fin_fila; fila++)
    {
        double b = b_max - fila * paso_b;

        for (int col = 0; col < ancho; col++)
        {
            double a = a_min + col * paso_a;
            int resultado = mandelbrot(a, b, max_iter);

            imagen[fila][col] = resultado;
            trabajo += resultado;
        }
    }

    // Juntamos las filas en P0
	if (rank != 0)
	{
		// Cada proceso envía las filas de su bloque
		for (int fila = inicio_fila; fila < fin_fila; fila++)
		{
			MPI_Send(imagen[fila], ancho, MPI_INT,
					 0, fila, MPI_COMM_WORLD);
		}
	}
	else
	{
		// P0 recibe el bloque de los demás procesos
		for (int origen = 1; origen < procesos; origen++)
		{
			int inicio_origen = origen * alto / procesos;
			int fin_origen = (origen + 1) * alto / procesos;

			for (int fila = inicio_origen; fila < fin_origen; fila++)
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

    // P0 obtiene la mayor duración entre todos los procesos
    MPI_Reduce(&tiempo_local, &tiempo_total,
               1, MPI_DOUBLE, MPI_MAX,
               0, MPI_COMM_WORLD);

    //  No medimos el tiempo de impresión ni la escritura
    printf("P%d: %lld iteraciones\n", rank, trabajo);

    int error_archivo = 0;

    if (rank == 0)
    {
		colormap_func_t paleta = select_colormap("hot");

		FILE *archivo = fopen("mandelbrot_mpi_bloques.ppm", "w");

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