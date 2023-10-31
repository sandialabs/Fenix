#include <stdlib.h>
#include <stdio.h>
#include <fenix.h>
void print_subset(Fenix_Data_subset *ss){
   printf("\tnum_blocks:\t %d\n", ss->num_blocks);
   printf("\tstride:\t\t %d\n", ss->stride);
   printf("\tspecifier:\t %d\n", ss->specifier);
   printf("\tstart_offsets:\t [");
   for(int i = 0; i < ss->num_blocks; i++){
      printf( (i==0) ? "%d" : ", %d", ss->start_offsets[i]);
   }
   printf("]\n");
   printf("\tend_offsets:\t [");
   for(int i = 0; i < ss->num_blocks; i++){
      printf( (i==0) ? "%d" : ", %d", ss->end_offsets[i]);
   }
   printf("]\n");
   printf("\tnum_repeats:\t [");
   for(int i = 0; i < ss->num_blocks; i++){
      printf( (i==0) ? "%d" : ", %d", ss->num_repeats[i]);
   }
   printf("]\n");
}

int test_subset_main(Fenix_Data_subset *ss, int num_blocks, 
      int *start_offsets, int *end_offsets, int *num_repeats){
   if(ss->num_blocks != num_blocks){
      //Wrong, and we don't want to keep checking or we might segfault
      return 0;
   }
  
   //Current implementation maintains ordering, so this assumes the 
   //tester knows the expected output order.
   int success = 1;
   for(int i = 0; (i < num_blocks) && success; i++){
      success = success && ss->start_offsets[i] == start_offsets[i];
      success = success && ss->end_offsets[i] == end_offsets[i];
      success = success && ss->start_offsets[i] == start_offsets[i];
   }

   return success;
}

int test_subset_create( Fenix_Data_subset *sub1, Fenix_Data_subset *sub2, 
      Fenix_Data_subset *sub3, int num_blocks, int stride, 
      int *start_offsets, int *end_offsets, int *num_repeats){
   int success = 1;
   success = success && test_subset_main(sub3, num_blocks, start_offsets, end_offsets, num_repeats);
   success = success && sub3->specifier == __FENIX_SUBSET_CREATE;
   success = success && sub3->stride == stride;

   if(!success){
      printf("ERROR!\n");
      printf("sub1: \n");
      print_subset(sub1);
      printf("sub2: \n");
      print_subset(sub2);
      printf("sub3: \n");
      print_subset(sub3);
   } else {
      printf("Success\n");
   }
   
   __fenix_data_subset_free(sub1);
   __fenix_data_subset_free(sub2);
   __fenix_data_subset_free(sub3);
   
   return !success; //return failure status
}

int test_subset_createv( Fenix_Data_subset *sub1, Fenix_Data_subset *sub2, 
      Fenix_Data_subset *sub3, int num_blocks, 
      int *start_offsets, int *end_offsets){
   int success = 1;
   int* zeros = calloc(num_blocks, sizeof(int));
   success = success && test_subset_main(sub3, num_blocks, start_offsets, end_offsets, zeros);
   free(zeros);
   success = success && sub3->specifier == __FENIX_SUBSET_CREATEV;

   if(!success){
      printf("ERROR!\n");
      printf("sub1: \n");
      print_subset(sub1);
      printf("sub2: \n");
      print_subset(sub2);
      printf("sub3: \n");
      print_subset(sub3);
   } else {
      printf("Success\n");
   }
   
   __fenix_data_subset_free(sub1);
   __fenix_data_subset_free(sub2);
   __fenix_data_subset_free(sub3);

   return !success;
}

int main(int argc, char **argv) {
   Fenix_Data_subset sub1;
   Fenix_Data_subset sub2;
   Fenix_Data_subset sub3;
   
   int failure = 0;

   printf("Testing equivalent create subsets of same size & location: ");
   Fenix_Data_subset_create(3, 2, 5, 5, &sub1);
   Fenix_Data_subset_create(3, 2, 5, 5, &sub2);
   __fenix_data_subset_merge(&sub1, &sub2, &sub3);
   failure += test_subset_create(&sub1, &sub2, &sub3, 1, 5, (int[]){2}, (int[]){5}, (int[]){2}); 
   
   printf("Testing equivalent create subsets, one within another: ");
   Fenix_Data_subset_create(1, 17, 20, 5, &sub1);
   Fenix_Data_subset_create(3, 12, 15, 5, &sub2);
   __fenix_data_subset_merge(&sub1, &sub2, &sub3);
   failure += test_subset_create(&sub1, &sub2, &sub3, 1, 5, (int[]){12}, (int[]){15}, (int[]){2});
   
   printf("Testing equivalent create subsets in non-overlapping, continuous regions: ");
   Fenix_Data_subset_create(1, 22, 25, 5, &sub1);
   Fenix_Data_subset_create(2, 12, 15, 5, &sub2);
   __fenix_data_subset_merge(&sub1, &sub2, &sub3);
   failure += test_subset_create(&sub1, &sub2, &sub3, 1, 5, (int[]){12}, (int[]){15}, (int[]){2});
   
   printf("Testing equivalent create subsets in non-overlapping, non-continuous regions: ");
   Fenix_Data_subset_create(1, 22, 25, 5, &sub1);
   Fenix_Data_subset_create(1, 12, 15, 5, &sub2);
   __fenix_data_subset_merge(&sub1, &sub2, &sub3);
   failure += test_subset_create(&sub1, &sub2, &sub3, 2, 5, (int[]){22, 12}, (int[]){25, 15}, (int[]){1,0});
   
   printf("Testing create subsets of same location: ");
   Fenix_Data_subset_create(1, 13, 15, 5, &sub1);
   Fenix_Data_subset_create(1, 12, 15, 5, &sub2);
   __fenix_data_subset_merge(&sub1, &sub2, &sub3);
   failure += test_subset_create(&sub1, &sub2, &sub3, 1, 5, (int[]){12}, (int[]){15}, (int[]){0});
   
   printf("Testing distinct create subsets with same stride: ");
   Fenix_Data_subset_create(1, 17, 19, 5, &sub1);
   Fenix_Data_subset_create(1, 12, 15, 5, &sub2);
   __fenix_data_subset_merge(&sub1, &sub2, &sub3);
   failure += test_subset_create(&sub1, &sub2, &sub3, 2, 5, (int[]){17, 12}, (int[]){19, 15}, (int[]){0, 0});
   
   printf("Testing distinct, overlapping create subsets with same stride: ");
   Fenix_Data_subset_create(1, 17, 19, 5, &sub1);
   Fenix_Data_subset_create(2, 12, 15, 5, &sub2);
   __fenix_data_subset_merge(&sub1, &sub2, &sub3);
   failure += test_subset_create(&sub1, &sub2, &sub3, 1, 5, (int[]){12}, (int[]){15}, (int[]){1});
   
   printf("Testing distinct create subsets with unique stride: ");
   Fenix_Data_subset_create(1, 17, 19, 6, &sub1);
   Fenix_Data_subset_create(1, 12, 15, 5, &sub2);
   __fenix_data_subset_merge(&sub1, &sub2, &sub3);
   failure += test_subset_createv(&sub1, &sub2, &sub3, 2, (int[]){17, 12}, (int[]){19, 15});

   printf("Testing distinct overlapping create subsets with unique stride: ");
   Fenix_Data_subset_create(1, 13, 16, 6, &sub1);
   Fenix_Data_subset_create(1, 12, 15, 5, &sub2);
   __fenix_data_subset_merge(&sub1, &sub2, &sub3);
   failure += test_subset_createv(&sub1, &sub2, &sub3, 1, (int[]){12}, (int[]){16});

   printf("Testing complex createv subsets: ");
   Fenix_Data_subset_createv(4, (int[]){1, 4, 21, 23}, (int[]){2, 17, 25, 26}, &sub1);
   Fenix_Data_subset_createv(3, (int[]){0, 18, 30}, (int[]){1, 19, 30}, &sub2);
   __fenix_data_subset_merge(&sub1, &sub2, &sub3);
   failure += test_subset_createv(&sub1, &sub2, &sub3, 4, (int[]){0, 4, 21, 30}, (int[]){2, 19, 26, 30});

   printf("Testing complex create and createv together: ");
   Fenix_Data_subset_create(4, 11, 13, 10, &sub1);
   Fenix_Data_subset_createv(3, (int[]){0, 12, 31}, (int[]){1, 20, 31}, &sub2);
   __fenix_data_subset_merge(&sub1, &sub2, &sub3);
   failure += test_subset_createv(&sub1, &sub2, &sub3, 4, (int[]){11, 31, 41, 0}, (int[]){23, 33, 43, 1});


   Fenix_Data_subset_delete(&sub1);

   return failure;
}
