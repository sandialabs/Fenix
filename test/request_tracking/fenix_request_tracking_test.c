#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#ifndef RTT_NO_FENIX
#include <fenix.h>
#endif

int main(int argc, char **argv)
{
    int i;
    int isends = 1000;
    int its = 50;
    int it;
    MPI_Comm newcomm;

    MPI_Init(&argc, &argv);

#ifndef RTT_NO_FENIX
    int fenix_status;
    int error;
    int spare_ranks = 1;
    Fenix_Init(&fenix_status, MPI_COMM_WORLD, &newcomm, &argc, &argv, spare_ranks, 0, MPI_INFO_NULL, &error);
#else
    #warning "no fenix"
    MPI_Comm_dup(MPI_COMM_WORLD, &newcomm);
#endif

    int rank, size;
    MPI_Comm_rank(newcomm, &rank);
    MPI_Comm_size(newcomm, &size);
    printf("Hello world from rank %d/%d\n", rank, size);

    int *bufs = (int *)malloc(isends*sizeof(int));
    int *bufs_recv = (int *)malloc(isends*sizeof(int));
    MPI_Request *reqs  = (MPI_Request *)malloc(isends*sizeof(MPI_Request));
    MPI_Request *reqs_recv  = (MPI_Request *)malloc(isends*sizeof(MPI_Request));
    MPI_Barrier(newcomm);
    MPI_Barrier(newcomm);
    MPI_Barrier(newcomm);
    double tstart;
    MPI_Request r;
    for(it=0 ; it<its ; it++) {
        if(it==2)
            tstart = MPI_Wtime();
        // for(i=0 ; i<isends ; i++)
        //     MPI_Isend(&(bufs[i]), 1, MPI_INT, (rank+1)%size, 0, newcomm, &(reqs[i]));
        for(i=0 ; i<isends ; i++) {
            if(0 && rank==0 && it==10 && i==10) {
                printf("Killing self\n");
                kill(getpid(), 9);
            }
            MPI_Isend(&(bufs[i]), 1, MPI_INT, (rank+1)%size, 0, newcomm, &r);
            memcpy(&(reqs[i]), &r, sizeof(MPI_Request));
        }
        for(i=0 ; i<isends ; i++)
            MPI_Irecv(&(bufs_recv[i]), 1, MPI_INT, (rank-1)%size, 0, newcomm, &(reqs_recv[i]));
        MPI_Waitall(isends, reqs, MPI_STATUSES_IGNORE);
        MPI_Waitall(isends, reqs_recv, MPI_STATUSES_IGNORE);
    }
    double time = MPI_Wtime()-tstart;
    printf("time taken per iteration (%d isends and %d irecvs): %f\n", isends, isends, time/(double)(its-2));
    free(bufs);
    free(bufs_recv);
    free(reqs);
    free(reqs_recv);

#ifndef RTT_NO_FENIX
    Fenix_Finalize();
#endif
    MPI_Finalize();
    return 0;
}
