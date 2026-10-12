#ifndef TIMER_H
#define TIMER_H

#include <mpi.h>

/* función encargada de medir el tiempo transcurrido usando MPI_Wtime*/

// toma el tiempo actual

static inline double timer_now(void){
    return MPI_Wtime();
}

// teniendo dos tiempos, retorna la diferencia

static inline double timer_transcurrido(double t_inicio, double t_fin){
    return t_fin - t_inicio;
}

#endif 