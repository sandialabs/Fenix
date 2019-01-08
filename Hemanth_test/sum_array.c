#include <fenix.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

const int kKillID = 2;

int main(int argc, char **argv) {

   if (argc < 3) {
      printf("Usage: my_test <num spare processes> <array size>\n");
      exit(0);
   }

   int fenix_role, error, fenix_comm_size, fenix_rank;
   int old_world_size, old_rank, flag, recovered;

   int global_sum = 0;
   int spare_ranks = atoi(argv[1]);
   int arr_size = atoi(argv[2]);

   int test_arr[arr_size];

   fenix_comm_size = old_world_size = old_rank = 0;
   fenix_rank = 0;
   error = 0;

   recovered = 0;

   /* Initialize MPI */
   MPI_Init(&argc, &argv);
   MPI_Barrier(MPI_COMM_WORLD); //idk what this is for

   /* Declare communicators to be used */
   MPI_Comm world_comm;
   MPI_Comm new_comm;
   MPI_Info info = MPI_INFO_NULL;

   /* Obtain the size and rank of the current communicator */
   MPI_Comm_dup(MPI_COMM_WORLD, &world_comm);
   MPI_Comm_size(world_comm, &old_world_size);
   MPI_Comm_rank(world_comm, &old_rank); 

   /* Print how many spare ranks are available, and which rank
    * will be killed
    */ 
   if (old_rank == 0) {
      printf("\nExecuting the program with %d spare ranks.  Rank \
%d will be killed.\n", spare_ranks, kKillID);
      printf("Printing information for ndx data stores.\n");
   }
  
   int sum = 0;

   /* Initialize the Fenix Interface */
   /* When a rank fails and is repaired by Fenix,
    * it will restart here 
    */ 
 
   /*Hemanth: Commented out call to Fenix_Init */
   /*For data_recover_only, we're not dealing with resilient communicator, rather plain old world_comm */
   Fenix_Data_recovery_init();
   /*
   Fenix_Init(&fenix_role, world_comm, &new_comm, &argc, &argv,
              spare_ranks, 0, info, &error);

   MPI_Comm_size(new_comm, &fenix_comm_size);
   MPI_Comm_rank(new_comm, &fenix_rank); 
   */

   fenix_comm_size = old_world_size;
   fenix_rank = old_rank;
   
   int chunk_size = arr_size / fenix_comm_size;
   int start = fenix_rank * chunk_size;
   int end = start + chunk_size;


   int ndx;

   /*Hemanth: Creating group with world_comm instead of new_comm
   Fenix_Data_group_create(24, new_comm, 0, 0);
   */

   Fenix_Data_group_create(24, world_comm, 0, 0);

   if (!error && fenix_role == FENIX_ROLE_INITIAL_RANK) {
      for (ndx = 0; ndx < arr_size; ndx++) {
         test_arr[ndx] = ndx;
      }
      ndx = start;

      /* Create fenix members for ndx, test_arr, and sum */
      Fenix_Data_member_create(24, 90, (void *)&ndx, 1, MPI_INT);
      Fenix_Data_member_create(24, 91, (void *)test_arr, arr_size, MPI_INT);
      Fenix_Data_member_create(24, 92, (void *)&sum, 1, MPI_INT);

   } else if (!error) {

      /* Upon failure, recover the failed rank's stored data */
      Fenix_Data_member_restore(24, 90, (void *)&ndx, 1, MPI_INT, 0);
      Fenix_Data_member_restore(24, 91, (void *)test_arr, arr_size, MPI_INT, 0);
      Fenix_Data_member_restore(24, 92, (void *)&sum, 1, MPI_INT, 0);

      recovered = 1;

      /* Set the data for future stores */
      Fenix_Data_member_attr_set(24, 90, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
                                 (void *)&ndx, &flag);
      Fenix_Data_member_attr_set(24, 91, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
                                 (void *)test_arr, &flag);
      Fenix_Data_member_attr_set(24, 92, FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER,
                                 (void *)&sum, &flag);

   } else if (error) {
      printf("fenix error\n");
   }

   /* Sum the contents of an array in parallel
    *
    * Array size of 60
    * 6 ranks
    * Each rank takes 10 indices and sums their content
    * Rank 0 takes 0-9, rank 1 takes 10-19, etc
    *
    */ 
   Fenix_Data_member_store(24, 91);
   for (; ndx < end; ndx++) {
      Fenix_Data_member_store(24, 90);
      /*if (fenix_rank == 2 && recovered == 0 && ndx == 21) {
   
         printf("\nKilling rank 2 to be recovered by Fenix.\n");
         printf("Rank 2 lost its ndx value of %d.\n", ndx);
         pid_t pid = getpid();
         kill(pid, SIGKILL);
      }*/

      Fenix_Data_member_store(24, 92);
      sum = sum + test_arr[ndx];
      if (fenix_rank == 2) {
         printf("My sum is: %d\n", sum);
      }
      Fenix_Data_commit_barrier(24, NULL);

      /* Hemanth: Commenting killing of an actual MPI process
      if (fenix_rank == 2 && recovered == 0 && ndx == 21) {
   
         printf("\nKilling rank 2 to be recovered by Fenix.\n");
         printf("Rank 2 lost its ndx value of %d.\n", ndx);
         pid_t pid = getpid();
         kill(pid, SIGKILL);
      }
      */
   }

   /* Compute the global sum from the sums of each rank */
   MPI_Reduce(&sum, &global_sum, 1, MPI_INT, MPI_SUM, 0, new_comm);
   

   /* Finalize Fenix and MPI */
   /* Hemanth: Commenting out Fenix_Finalize
   Fenix_Finalize();
   */
   MPI_Finalize();


   /* Print our computed sum, and determine if it matches the expected sum */
   if (fenix_rank == 0) {
      printf("\nArray Size: %d\n", arr_size);
      printf("Global Sum: %d\n", global_sum);
   }

   if (fenix_rank == 0) {
      arr_size--;
      int expected_sum = (arr_size * (arr_size + 1)) / 2;
      printf("Expected Global Sum: %d\n", expected_sum);
   }

   return 0;
}
