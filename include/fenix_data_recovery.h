/*
 * Header file containing user-level functions for the Fenix
 * data recovery interface.
 */

#ifndef __FENIX_DATA_RECOVERY__
#define __FENIX_DATA_RECOVERY__

#include "fenix_data_group.h"
#include "fenix_data_member.h"
#include "fenix_util.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>

#define GROUP_ENTRY_ATTR_SIZE 4  //was 5, removed depth so it's now 4
#define NUM_MEMBER_ATTR_SIZE 3

#define STORE_DATA_TAG 2003

int __Fenix_Data_group_create(int group_id, MPI_Comm comm,
                            int start_time_stamp, int depth);
int __Fenix_Data_member_create(int group_id, int member_id,
                             void *source_buffer, int count,
                             MPI_Datatype datatype);

int __Fenix_Data_group_delete(int group_id);
int __Fenix_Data_member_delete(int group_id, int member_id);

int __Fenix_Data_member_store(int group_id, int member_id);  //no subset specifier
/*int __Fenix_Data_member_restore(int group_id, int member_id, void *data,
                                int count, int timestamp);*/
int __Fenix_Data_member_restore(int group_id, int member_id, void *data,
                                int count, MPI_Datatype datatype, int timestamp);

int __Fenix_Data_commit_barrier(int group_id, int *time_stamp);

// extern fenix_group_t *__fenix_g_data_recovery;

int __fenix_member_set_attribute(int group_id, int member_id, 
                                 int attributename, void *attributevalue,
                                 int *flag);

int __send_metadata(int current_rank, int in_rank, MPI_Comm comm);
int __send_group_data(int current_rank, int in_rank,
                      fenix_group_entry_t *group_entry, MPI_Comm comm);
int __recover_metadata(int current_rank, int out_rank, MPI_Comm comm);
int __recover_group_data(int current_rank, int out_rank,
                         fenix_group_entry_t *group_entry, MPI_Comm comm);

int __fenix_search_member_id(int group_index, int key);
int __fenix_find_next_member_position(fenix_member_t *member);

int __fenix_join_group(fenix_group_t *group, fenix_group_entry_t *entry, MPI_Comm);
int __fenix_join_member(fenix_member_t *member, fenix_member_entry_t *entry, MPI_Comm);

#endif
