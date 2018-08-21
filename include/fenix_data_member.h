/*
 *
 *
 *
 */

#ifndef __FENIX_DATA_MEMBER_H__
#define __FENIX_DATA_MEMBER_H__

#include <mpi.h>
//#include "fenix_data_group.h"
//#include "fenix_data_packet.h"
#include "fenix_util.h"
#include "fenix_data_buffer.h"

#define __FENIX_DEFAULT_MEMBER_SIZE 512 //look at this later

/* Need to determine which fields are necessary
 * and which ones need to be removed because 
 * versioning will not be implemented
 */
typedef struct __fenix_member_entry {
   int member_id;
   enum states state;  //states from fenix util
   //fenix_version_t *version;
   void *user_data;
   MPI_Datatype current_datatype;
   int datatype_size;  //size of the MPI datatype in bytes

   fenix_buffer_entry_t local_entry;  //main local and remote buffers
   fenix_buffer_entry_t remote_entry; 

   fenix_buffer_entry_t backup_local_entry;  //backup local and remote buffers
   fenix_buffer_entry_t backup_remote_entry;

   int current_size;   //not entirely sure what this is
   int current_count;  //size of data 
   //int current_rank;   //local rank 
   
   int stored; //indicate if we have stored this member yet

   /* flags for main and backup buffers */
   int main_flag, backup_flag;

   int remote_main_flag, remote_backup_flag;

   /*int current_count;
   int current_size;
   int current_rank;
   int remote_rank;
   int remote_rank_front;
   int remote_rank_back;*/
} fenix_member_entry_t;

typedef struct __fenix_member {
   size_t count;
   int temp_count;
   size_t total_size;
   fenix_member_entry_t *member_entry;
} fenix_member_t;

typedef struct __member_store_packet {
   //int rank;
   MPI_Datatype datatype;
   int entry_count;
   size_t entry_size;
   //int entry_real_count;
   //int num_blocks;
} fenix_member_store_packet_t;

typedef struct __member_entry_packet {
   int member_id;
   enum states state;
   MPI_Datatype current_datatype;
   int datatype_size;

   int current_size;
   int current_count;
} fenix_member_entry_packet_t;

typedef struct __buffer_flag_packet {
   int main_flag;
   int backup_flag;
} fenix_member_buffer_flag_packet_t;

typedef struct __member_type_packet {
   int dsize;
   MPI_Datatype dtype;
} fenix_member_type_packet_t;

fenix_member_t *__fenix_data_member_init();

void __fenix_data_member_destroy(fenix_member_t *member);
void __fenix_ensure_member_capacity(fenix_member_t *member);
//void __fenix_data_member_reinit(fenix_member_t *m, fenix_metadata_packet_t packet, int state);
/* Likely not needed due to versioning */
void __fenix_ensure_version_capacity_from_member(fenix_member_t *member);

//int __fenix_search_member_id(int group_index, int key);

#endif
