/*
 *
 *
 *
 */

#ifndef __FENIX_DATA_GROUP_H__
#define __FENIX_DATA_GROUP_H__

#include <mpi.h>
#include "fenix_data_member.h"
//#include "fenix_data_packet.h"
#include "fenix_util.h"

#define __FENIX_DEFAULT_GROUP_SIZE 32

typedef struct __fenix_group_entry{
   int group_id;
   MPI_Comm comm;
   int comm_size;
   int current_rank;
   int in_rank;
   int out_rank;
   int timestart;
   int timestamp;
   //int depth;   //versioning not implemented
   int rank_separation;

   /* flags for main and backup buffers */
   //int main_flag;
   //int backup_flag;

   /* subject to change */
   enum states state;
   int recovered;

   fenix_member_t *member;
} fenix_group_entry_t;

typedef struct __fenix_group {
   size_t count;
   size_t total_size;
   fenix_group_entry_t *group_entry;
} fenix_group_t;

/* struct used for sending group_entry metadata */
typedef struct __group_entry_packet {
   int group_id;
   int timestamp;
   int depth;
   int rank_separation;
   enum states state;
} fenix_group_entry_packet_t;

/*struct used for sending group metadata */
/*typedef struct __metadata_packet {
   size_t count;
   size_t total_size;
} fenix_metadata_packet_t;*/

extern fenix_group_t *fenix_g_data_recovery;

fenix_group_t * __fenix_data_group_init();

void __fenix_data_group_destroy(fenix_group_t *group);
//void __fenix_data_group_reinit(fenix_group_t *group, fenix_metadata_packet_t packet);
void __fenix_ensure_group_capacity(fenix_group_t *group);
int __fenix_search_groupid(int key, fenix_group_t *group);
int __fenix_find_next_group_position(fenix_group_t *group);

#endif
