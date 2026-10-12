/*
   paralelización del conjunto de Mandelbort con esquema dinámico maestro-trabajador.
   El maestro reparte bloques de filas a medidaque los trabajadores los solicitan
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mandelbrot.h"
#include "timer.h"
#include "colormap.h"


/* Etiquetas para los mensajes MPI. */
#define TAG_PEDIDO    1
#define TAG_TAREA     2
#define TAG_RESULTADO 3
#define TAG_FIN       4

/*Muestra por pantalla como se usa el programa.*/
static void uso(const char *prog) {
    fprintf(stderr,
            "Uso: mpiexec -n <P> %s [ancho] [alto] [max_iter] [filas_bloque] [hot|magma|ocean]\n",
            prog);
}

/*Lee los argumentos ingresados por el suusario y los guarda en la struct Config*/
static int leer_argumentos(int argc, char **argv, Config *cfg) {
    if (argc > 1) cfg->width       = atoi(argv[1]);
    if (argc > 2) cfg->height      = atoi(argv[2]);
    if (argc > 3) cfg->max_iter    = atoi(argv[3]);
    if (argc > 4) cfg->filas_bloque = atoi(argv[4]);

    if (cfg->width <= 1 || cfg->height <= 1 || cfg->max_iter <= 0 ||
        cfg->filas_bloque <= 0) {
        return -1;
    }
    return 0;
}

/*
 * Calcula las nfilas filas consecutivas iniciando en fila_inicio
 * Llama a calcular_fila (mandenbort.h) y calcula als iteraciones de 
 * Mandenbort para cada fila.
 */
static void procesar_bloque(const Config *cfg, int fila_inicio, int nfilas,int *local) {
    for (int j = 0; j < nfilas; ++j) {
        calcular_fila(cfg, fila_inicio + j, &local[j * cfg->width]);
    }
}


int main(int argc, char **argv) {
    int rango, n_procesos;
    Config cfg = config_default();

    MPI_Init(&argc, &argv);                     //inicio de MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rango);      //guardo en rank el identificador del proceso
    MPI_Comm_size(MPI_COMM_WORLD, &n_procesos); //guarda el total de procesos

    if (n_procesos < 2) {
        if (rango == 0) {
            fprintf(stderr,
                    "Este programa requiere al menos 2 procesos MPI.\n");
        }
        MPI_Finalize();
        return 1;
    }

    colormap_func_t paleta = hot_colormap;
    const char *nombre_paleta = "hot"; //el colormap es hot por defecto
    //solo el proceso 0 lee los argumentos enviados
    if (rango == 0) {
        if (leer_argumentos(argc, argv, &cfg) != 0) {
            uso(argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        if (argc > 5) {
            paleta = select_colormap(argv[5]);
            nombre_paleta = argv[5];
        }
    }

    //el maestro difunde la configuración a todos los trabajadores. 
    MPI_Bcast(&cfg, sizeof(Config), MPI_BYTE, 0, MPI_COMM_WORLD);

    double t_inicio, t_fin;
    double tiempo_local;
    double tiempo_total = 0.0;

    if (rango == 0) {
        /* ---------- MAESTRO ---------- */
        //reservo la memoria necesaria para la imagen completa
        int *imagen = (int *)calloc((size_t)cfg.width * cfg.height, sizeof(int));
        if (imagen == NULL) {
            fprintf(stderr, "Maestro: error de memoria.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        //calculo para obtener la cantidad de bloeques de filas
        int total_tareas = (cfg.height + cfg.filas_bloque - 1) / cfg.filas_bloque;
        int tarea_actual = 0;
        int trabajadores_activos = n_procesos - 1;

        /*
         * Buffer para recibir resultados:
         * [0] fila_inicio 
         * [1] cantidad_filas
         * [2..] datos de iteraciones.
         */
        int max_recv = 2 + cfg.filas_bloque * cfg.width;
        int *recvbuf = (int *)malloc((size_t)max_recv * sizeof(int));
        if (recvbuf == NULL) {
            fprintf(stderr, "Maestro: error de memoria.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        t_inicio = timer_now();

        /* ---------- BUCLE PRINCIPAL ---------- 
         * Bucle principal: mientras haya trabajadores activos,
         * recibir pedidos o resultados y asignar nuevas tareas.
         */
        while (trabajadores_activos > 0) {
            MPI_Status status;
            //espera mensaje de cualquier trabajador. Status indica quién envió el mensaje y con qué tag
            MPI_Recv(recvbuf, max_recv, MPI_INT, MPI_ANY_SOURCE,
                     MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int origen = status.MPI_SOURCE;
            
            //dependiendo de si el mensaje es un resultado procede a colocarlo en la imagen final
            if (status.MPI_TAG == TAG_RESULTADO) {
                int fila_inicio = recvbuf[0];
                int nfilas      = recvbuf[1];
                for (int j = 0; j < nfilas; ++j) {
                    memcpy(&imagen[(fila_inicio + j) * cfg.width],
                           &recvbuf[2 + j * cfg.width],
                           (size_t)cfg.width * sizeof(int));
                }
            }

            /* si quedan tareas: envía al trabajador cuáles filas a calcular (TAG_TAREA)
             * si no hay tareas: envía TAG_FIN para que ese trabajador termine
             */
            if (tarea_actual < total_tareas) {
                int fila_inicio = tarea_actual * cfg.filas_bloque;
                int nfilas = cfg.filas_bloque;
                if (fila_inicio + nfilas > cfg.height) {
                    nfilas = cfg.height - fila_inicio;
                }
                int sendbuf[2] = {fila_inicio, nfilas};
                MPI_Send(sendbuf, 2, MPI_INT, origen, TAG_TAREA,
                         MPI_COMM_WORLD);
                tarea_actual++;
            } else {
                int sendbuf[2] = {-1, -1};
                MPI_Send(sendbuf, 2, MPI_INT, origen, TAG_FIN,
                         MPI_COMM_WORLD);
                trabajadores_activos--;
            }
        }
        //se mide el tiempo y se procede a imprimirlo, guardar la imagen y liberar la memoria
        t_fin = timer_now();
        tiempo_local = timer_transcurrido(t_inicio, t_fin);

        MPI_Reduce(&tiempo_local, &tiempo_total,
                1, MPI_DOUBLE, MPI_MAX,
                0, MPI_COMM_WORLD);

        printf("Mandelbrot paralelo - maestro/trabajador\n");
        printf("Procesos: %d, Resolucion: %d x %d, max_iter: %d, "
               "tarea: %d filas\n",
               n_procesos, cfg.width, cfg.height, cfg.max_iter,
               cfg.filas_bloque);
        printf("Tiempo: %.6f segundos\n", tiempo_total);

        if (GENERAR_SALIDAS)
        {
            if (escribir_iteraciones("resultados/dinamico_iter.txt", cfg.width, cfg.height, imagen) != 0)
            {
                fprintf(stderr, "Error al guardar las iteraciones\n");
            }

            char nombre[128];
            snprintf(nombre, sizeof(nombre), "mandelbrot_dinamico_%dx%d_%s.ppm",
                    cfg.width, cfg.height, nombre_paleta);
            if (escribir_ppm(nombre, &cfg, imagen, paleta) == 0) {
                printf("Imagen guardada en %s\n", nombre);
            }
        }
        free(imagen);
        free(recvbuf);

    } else {
        /* ---------- TRABAJADOR ---------- */
        int max_filas = cfg.filas_bloque;

        /* Se reserva memoria para:
         * local: guarda la memoria necesaria para guardar las filas que calcula
         * sendbuf: enviar fila_inicio, cantdad y los datos al maestro
         */
        int *local = (int *)malloc((size_t)cfg.width * max_filas * sizeof(int));
        int *sendbuf = (int *)malloc((2 + (size_t)cfg.width * max_filas) *
                                     sizeof(int));
        if (local == NULL || sendbuf == NULL) {
            fprintf(stderr, "Trabajador %d: error de memoria.\n", rango);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        long long trabajo = 0;

        MPI_Barrier(MPI_COMM_WORLD);
        t_inicio = timer_now();

        // pedir primera tarea
        int dummy = 0;
        MPI_Send(&dummy, 1, MPI_INT, 0, TAG_PEDIDO, MPI_COMM_WORLD);

        while (1) {
            int tarea[2];
            MPI_Status status;
            MPI_Recv(tarea, 2, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD,
                     &status);
            // espera el mensaje del maestro, si recibe TAG_FIN termina
            if (status.MPI_TAG == TAG_FIN) {
                break;
            }
            
            int fila_inicio = tarea[0];
            int nfilas      = tarea[1];
            // recibe la configuración, las filas a calcular y las procesa
            procesar_bloque(&cfg, fila_inicio, nfilas, local);

            for (int j = 0; j < nfilas; j++)
            {
                for (int col = 0; col < cfg.width; col++)
                {
                    trabajo += local[j * cfg.width + col];
                }
            }

            /* Enviar resultado y pedir siguiente tarea. */
            sendbuf[0] = fila_inicio;
            sendbuf[1] = nfilas;
            for (int j = 0; j < nfilas; ++j) {
                memcpy(&sendbuf[2 + j * cfg.width],
                       &local[j * cfg.width],
                       (size_t)cfg.width * sizeof(int));
            }
            /* se prepara el mensaje con el resultado y lo envía al maestro, este MPI_SEND
             * también funciona como pedido de la siguiente tarea
             * */
            MPI_Send(sendbuf, 2 + nfilas * cfg.width, MPI_INT, 0,
                     TAG_RESULTADO, MPI_COMM_WORLD);
        }

        t_fin = timer_now();
        tiempo_local = timer_transcurrido(t_inicio, t_fin);

        MPI_Reduce(&tiempo_local, &tiempo_total,
                1, MPI_DOUBLE, MPI_MAX,
                0, MPI_COMM_WORLD);

        printf("Trabajador %d: %lld iteraciones\n", rango, trabajo);

        free(local);
        free(sendbuf);
    }

    MPI_Finalize();
    return 0;
}
