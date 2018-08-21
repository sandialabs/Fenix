/*
 * Implementation of core Fenix data recovery interface functions.
 */

#include <mpi.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "fenix_data_recovery.h"
#include "fenix_util.h"
//#include "fenix_metadata.h"
#include "fenix_ext.h"
#include "fenix_opt.h"

int __Fenix_Data_group_create(int group_id, MPI_Comm comm,
                            int start_time_stamp, int depth) {

   int rtn, new_group, group_position, remote_need_recovery, next_entry;
   fenix_group_t *group;
   fenix_group_entry_t *group_entry;
   MPI_Status status;

   /* find an available group_index or if this index is in use */
   int group_index = __fenix_search_groupid(group_id, fenix.data_recovery);
   
   /* Check if the timestamp is valid.
    * Should always be 0, since versioning is not implemented?
    * May not even have to worry about timestamp value.
    */
   if (start_time_stamp < 0) {
      fprintf(stderr, "Error Fenix_Data_group_create: start_time_stamp <%d> \
must be greater than or equal to zero\n", start_time_stamp);
      rtn = -1;  //set this and all other rtn values to constants later
   } 
   /* Check if depth is valid */
   else if (depth < -1) {
      fprintf(stderr, "Error Fenix_Data_group_create: depth <%d> \
must be greater than or equal to -1\n", depth);
      rtn = -1;
   } 
   /* Check the need for data recovery. */
   else {
      
      /* If the group data is already created */
      group = fenix.data_recovery;
      
      /* yet to implmenet this function */
      // __fenix_ensure_group_capacity(group);
      
      /* determine if the group index exists yet or not */
      if (group_index == -1) {
         new_group = 1;
      } else {
         new_group = 0;
      }

      /* if the group is new, initialize it */
      /* this is either a new or recovered process */
      if (new_group == 1) {

         //fprintf(stderr, "This is a new group!\n");
         group->count += 1;
         
         
         /* find the next available group entry */
         next_entry = __fenix_find_next_group_position(group);
         group_entry = &(group->group_entry[next_entry]);

         /* Initialize the entry's metadata */
         group_entry->group_id = group_id;
         group_entry->comm = comm;
         group_entry->timestart = start_time_stamp;  //may not need 
         group_entry->timestamp = start_time_stamp;  //timestamp info
         group_entry->state = OCCUPIED;

         /* If the rank is recovered, set the recovery field to 1 */
         if (fenix.role == FENIX_ROLE_RECOVERED_RANK) {
            group_entry->recovered = 1;
         } else {
            group_entry->recovered = 0;
         }

         /* Initialize rank separation.
          * Multiple policies will be implemented in the future,
          * but for the current version we will use this one.
          *
          * Copying this exactly from current implementation.
          *
          */

         group_entry->rank_separation = 1;
      }
      /* Group has already been created.  Renew MPI Comm */ 
      else {

         group_entry = &(group->group_entry[group_index]);
         group_entry->comm = comm;
      }

      /* Initialize the rest of the entry's metadata */
      group_entry->current_rank = __fenix_get_current_rank(group_entry->comm);
      group_entry->comm_size = __fenix_get_world_size(group_entry->comm);

      /* assign buddy ranks for each rank to send and receive data */
      group_entry->in_rank = (group_entry->current_rank + group_entry->comm_size
                             - group_entry->rank_separation) %
                             group_entry->comm_size;
      group_entry->out_rank = (group_entry->current_rank + group_entry->comm_size
                             + group_entry->rank_separation) %
                             group_entry->comm_size;
      
      rtn = (__fenix_join_group(group, group_entry, comm) != 1) ? FENIX_SUCCESS : FENIX_ERROR_GROUP_CREATE;
   }
   return rtn;
}

int __Fenix_Data_member_create(int group_id, int member_id,
                             void *source_buffer, int count,
                             MPI_Datatype datatype) {

   int rtn = -1;
   
   /* Find the requested group and member indices (determine if they exist) */
   int group_index = __fenix_search_groupid(group_id, fenix.data_recovery); 
   int member_index = __fenix_search_member_id(group_index, member_id);

   if (group_index == -1) {
      fprintf(stderr, "ERROR: group_id <%d> does not exist\n", group_id);
      rtn = -1;  //should be FENIX_ERROR_INVALID_GROUPID
   } else if (member_index != -1) {
      fprintf(stderr, "ERROR: member_id <%d> already exists\n", member_id);
      rtn = -1;  //should be FENIX_ERROR_INVALID_MEMBERID
   } else {

      int next_member_position;
      MPI_Status status;

      /* Point to group member and initialize member entry */
      fenix_group_t *group = fenix.data_recovery;
      fenix_group_entry_t *group_entry = &(group->group_entry[group_index]);
      fenix_member_t *member = group_entry->member;
      fenix_member_entry_t *member_entry = NULL;
      
      /* check if member entry needs to be made larger */
      //__fenix_ensure_member_capacity  //still need to implement

      member->count += 1;

      /* Obtain next available member_entry slot */
      next_member_position = __fenix_find_next_member_position(member);
      
      /* Point member_entry to the obtained member_entry slot */
      member_entry = &(member->member_entry[next_member_position]);

      /* Initialize member metadata */
      member_entry->member_id = member_id;
      member_entry->state = OCCUPIED;
      member_entry->current_datatype = datatype;
      member_entry->current_count = count;
      
      int d_size;
      MPI_Type_size(datatype, &d_size);

      member_entry->datatype_size = member_entry->current_size = d_size;   //check this line

      /* Point member entry user_data to provided data */
      member_entry->user_data = source_buffer;

      /* assign local and remote buffers */
      fenix_buffer_entry_t *l_entry = &(member_entry->local_entry);  //main buffers
      fenix_buffer_entry_t *r_entry = &(member_entry->remote_entry);

      fenix_buffer_entry_t *b_l_entry = &(member_entry->backup_local_entry);  //backup buffers
      fenix_buffer_entry_t *b_r_entry = &(member_entry->backup_remote_entry);

      /* determine why we need these buffers for remote and local entries */
      /* allocate main buffers */
      l_entry->data = (void *) s_malloc(member_entry->datatype_size * count);
      r_entry->data = (void *) s_malloc(member_entry->datatype_size * count);

      /* allocate backup buffers */
      b_l_entry->data = (void *) s_malloc(member_entry->datatype_size * count);
      b_r_entry->data = (void *) s_malloc(member_entry->datatype_size * count);
   
      /* need to call our global agreement function for joining */
      //similar to function call from group_create
      rtn = (__fenix_join_member(member, member_entry, group_entry->comm) != 1) ? FENIX_SUCCESS : FENIX_ERROR_MEMBER_CREATE;
   }

   return rtn;
}

int __Fenix_Data_member_store(int group_id, int member_id) {
  
   MPI_Status status;
   int rtn, group_index, member_index;
   int m_flag, b_flag, r_m_flag, r_b_flag;
   char *local_buff, *remote_buff;
   char *backup_local_buff, *backup_remote_buff;
   fenix_member_store_packet_t l_entry_packet, r_entry_packet;
   fenix_member_store_packet_t b_l_entry_packet, b_r_entry_packet;
   fenix_member_buffer_flag_packet_t l_flag_packet, r_flag_packet;
 
   rtn = -1;

   /* determine if the group_id and member_id alerady exist
      if so, assign the index of the storage space */
   group_index = __fenix_search_groupid(group_id, fenix.data_recovery);
   member_index = __fenix_search_member_id(group_index, member_id);
   
   if (group_index == -1) {
      fprintf(stderr, "ERROR: group_id <%d> does not exist\n", group_id);
      rtn = -1;  //should be FENIX_ERROR_INVALID_GROUPID
   } else if (member_index == -1) {
      fprintf(stderr, "ERROR: member_id <%d> does not exist\n", member_id);
      rtn = -1;  //should be FENIX_ERROR_INVALID_MEMBERID
   } else {

      /* point to group_entry and member_entry*/
      fenix_group_t *group = fenix.data_recovery;
      fenix_group_entry_t *group_entry = &(group->group_entry[group_index]);
      fenix_member_t *member = group_entry->member;
      fenix_member_entry_t *member_entry = &(member->member_entry[member_index]);

      /* main buffers */
      fenix_buffer_entry_t *l_entry = &(member_entry->local_entry);
      fenix_buffer_entry_t *r_entry = &(member_entry->remote_entry);

      /* backup buffers */
      fenix_buffer_entry_t *b_l_entry = &(member_entry->backup_local_entry);
      fenix_buffer_entry_t *b_r_entry = &(member_entry->backup_remote_entry);

      /* initialize the store packet */
      l_entry_packet.datatype = l_entry->datatype;
      l_entry_packet.entry_count = l_entry->count;
      l_entry_packet.entry_size = l_entry->datatype_size;

      /* send the local metadata to out_rank, and receive the remote metadata from in_rank */
      /* TODO - 0's should be STORE_SIZE_TAG 
       *
       * This Sendrecv currently serves no purpose besides allowing process recovery to intercept
       * upon an MPI call with a damage communicator.  Try to remove this.
       * */
      MPI_Sendrecv(&l_entry_packet, sizeof(fenix_member_store_packet_t), MPI_BYTE, 
                   group_entry->out_rank, 0, &r_entry_packet, 
                   sizeof(fenix_member_store_packet_t), MPI_BYTE, 
                   group_entry->in_rank, 0, group_entry->comm, &status);
     
      /* create a packet to send the current flags */ 
      l_flag_packet.main_flag = member_entry->main_flag;
      l_flag_packet.backup_flag = member_entry->backup_flag;

      /* send/recv local/remote buffer flags */
      MPI_Sendrecv(&l_flag_packet, sizeof(fenix_member_buffer_flag_packet_t),
                   MPI_BYTE, group_entry->out_rank, 0,
                   &r_flag_packet, sizeof(fenix_member_buffer_flag_packet_t), 
                   MPI_INT, group_entry->in_rank, 0,
                   group_entry->comm, &status);

      member_entry->remote_main_flag = r_flag_packet.main_flag;
      member_entry->remote_backup_flag = r_flag_packet.backup_flag;

      local_buff = (char *)l_entry->data;
      
      /* copy the current data into the backup entry, before updating the current data */
      if (l_entry->data != NULL && member_entry->stored == 1) {
         memcpy(b_l_entry->data, l_entry->data, member_entry->current_count * member_entry->datatype_size);
      }
      member_entry->stored = 1;

      /* store a copy of the local data */
      memcpy(l_entry->data, member_entry->user_data, member_entry->current_count * member_entry->datatype_size);

      /* most recent snapshot is no longer recoverable, until commit barrier;
       * use backup_buffer to restore data */
      member_entry->main_flag = 0;
      member_entry->backup_flag = 1;

      l_flag_packet.main_flag = member_entry->main_flag;
      l_flag_packet.backup_flag = member_entry->backup_flag;

      /* send/recv local/remote buffer flags */
      MPI_Sendrecv(&l_flag_packet, sizeof(fenix_member_buffer_flag_packet_t),
                   MPI_BYTE, group_entry->out_rank, 0,
                   &r_flag_packet, sizeof(fenix_member_buffer_flag_packet_t), 
                   MPI_INT, group_entry->in_rank, 0,
                   group_entry->comm, &status);

      member_entry->remote_main_flag = r_flag_packet.main_flag;
      member_entry->remote_backup_flag = r_flag_packet.backup_flag;
     
      /* This should only happen after a failure, as this memory has not been allocated
       * because that occurs in member_create.
       */
      if (r_entry->data != NULL) {
         r_entry->data = s_malloc(member_entry->current_count * member_entry->datatype_size);
      }
      if (b_r_entry->data != NULL) {
         b_r_entry->data = s_malloc(member_entry->current_count * member_entry->datatype_size);
      }

      /* place the outgoing data in a local buffer of chars */
      local_buff = (char *)l_entry->data;
      backup_local_buff = (char *)b_l_entry->data;

      /* assign local char buffer to the remote data */
      remote_buff = r_entry->data;
      backup_remote_buff = b_r_entry->data;
    
      /* send the local data to out_rank, receive the remote data from in_rank */
      /* TODO - 0's should be STORE_SIZE_TAG */
      MPI_Sendrecv((void *)local_buff, member_entry->current_count * member_entry->datatype_size,
                   MPI_BYTE, group_entry->out_rank, 0, (void *)remote_buff, 
                   member_entry->current_count * member_entry->datatype_size,
                   MPI_BYTE, group_entry->in_rank, 0, group_entry->comm, &status);

      /* send/receive the backup data */
      MPI_Sendrecv((void *)backup_local_buff, member_entry->current_count * member_entry->datatype_size,
                   MPI_BYTE, group_entry->out_rank, 0, (void *)backup_remote_buff, 
                   member_entry->current_count * member_entry->datatype_size,
                   MPI_BYTE, group_entry->in_rank, 0, group_entry->comm, &status);

      if (group_entry->current_rank == 2 && member_id == 90) {
         printf("\nFENIX STORE:\n\nCurrent rank %d sent its data to out rank %d\n"
             "and received data from in rank %d.\n"
             "Sent count of %d and datatype_size of %d.\n"
             "Data sent is %d and data received is %d.\n",
             group_entry->current_rank, group_entry->out_rank, 
             group_entry->in_rank, member_entry->current_count, member_entry->datatype_size,
             ((int *)local_buff)[0], ((int *)remote_buff)[0]);
      }
   }

   return 0;
}

int __Fenix_Data_member_restore(int group_id, int member_id, void *data, 
                                int count, MPI_Datatype datatype, int timestamp) {

   int rtn, group_index, member_index;
   int current_state, remote_state;
   int d_size;
   MPI_Status status;

   rtn = 0;
   
   group_index = __fenix_search_groupid(group_id, fenix.data_recovery);
   member_index = __fenix_search_member_id(group_index, member_id);

   MPI_Type_size(datatype, &d_size);

   if (group_index == -1) {
      fprintf(stderr, "ERROR Fenix_Data_member_restore: group_id <%d> does not exist\n", group_id);
      rtn = -1;
   } else {

      fenix_group_t *group = fenix.data_recovery;
      fenix_group_entry_t *group_entry = &(group->group_entry[group_index]);
      fenix_member_t *member = group_entry->member;
      fenix_member_entry_t *member_entry;

      if (member_index != -1) { /* Member entry is available (survivor rank) */
         member_entry = &(member->member_entry[member_index]);
      } else {  /* Member entry is missing (recovered rank) */

         /* Re-build the member entries */
         member_index = member->count;
         member_entry = &(member->member_entry[member->count]);
         member->count++;
         member_entry->state = NEEDFIX;
         member_entry->member_id = member_id;
         member_entry->current_count = count;

         member_entry->current_datatype = datatype;
         member_entry->datatype_size = member_entry->current_size = d_size;   //check this line

         /* NEW */
         member_entry->stored = 0;

         /* need to reallocate buffer memory.. also datatype size? */
      }
       
      current_state = member_entry->state;
      remote_state = 0;

      MPI_Sendrecv(&current_state, 1, MPI_INT, group_entry->out_rank, 0,
                   &remote_state, 1, MPI_INT, group_entry->in_rank, 0,
                   group_entry->comm, &status);

      if (current_state == NEEDFIX && remote_state == NEEDFIX) {
         //not supported, return error
         rtn = -1;
      } else if (current_state == OCCUPIED && remote_state == NEEDFIX) {
         /* I'm okay, but partner needs fix */

         /* send flags, put all in one send */
         int main_flag, backup_flag;
         int remote_main_flag, remote_backup_flag;
         fenix_member_buffer_flag_packet_t l_flag_packet;
         fenix_buffer_entry_t *r_entry, *b_r_entry;
         fenix_member_type_packet_t l_type_packet;

         l_flag_packet.main_flag = member_entry->remote_main_flag;
         l_flag_packet.backup_flag = member_entry->remote_backup_flag;

         MPI_Send(&l_flag_packet, sizeof(fenix_member_buffer_flag_packet_t),
                  MPI_BYTE, group_entry->in_rank, 0, group_entry->comm);

         if (member_entry->main_flag == 1 && member_entry->backup_flag == 0) {
            r_entry = &(member_entry->remote_entry);

            /* Send main (most recent) data */
            MPI_Send(r_entry->data, count * member_entry->datatype_size, datatype,
                     group_entry->in_rank, 0, group_entry->comm);

         } else if (member_entry->main_flag == 0 && member_entry->backup_flag == 1) {
            b_r_entry = &(member_entry->backup_remote_entry);

            /* Send backup data */
            MPI_Send(b_r_entry->data, count * member_entry->datatype_size, datatype,
                     group_entry->in_rank, 0, group_entry->comm);
         }
      } else if (current_state == NEEDFIX && remote_state == OCCUPIED) {
         /* I need fix, partner is okay */

         fenix_member_buffer_flag_packet_t r_flag_packet;
         fenix_member_type_packet_t r_type_packet;

         MPI_Recv(&r_flag_packet, sizeof(fenix_member_buffer_flag_packet_t),
                  MPI_BYTE, group_entry->out_rank, 0, group_entry->comm, &status);

         member_entry->main_flag = r_flag_packet.main_flag;
         member_entry->backup_flag = r_flag_packet.backup_flag;

         fenix_buffer_entry_t *l_entry = &(member_entry->local_entry);
         fenix_buffer_entry_t *r_entry = &(member_entry->remote_entry);

         /* reallocate memory for the main and backup local/remote buffers */
         l_entry->data = (void *) s_malloc(member_entry->datatype_size * count);
         r_entry->data = (void *) s_malloc(member_entry->datatype_size * count);

         fenix_buffer_entry_t *b_l_entry = &(member_entry->backup_local_entry);
         fenix_buffer_entry_t *b_r_entry = &(member_entry->backup_remote_entry);

         b_l_entry->data = (void *) s_malloc(member_entry->datatype_size * count);
         b_r_entry->data = (void *) s_malloc(member_entry->datatype_size * count);

         if (member_entry->main_flag == 1 && member_entry->backup_flag == 0) {
            /* most recent commit */

            MPI_Recv(l_entry->data, member_entry->datatype_size * member_entry->current_count, 
                     MPI_BYTE, group_entry->out_rank, 0,
                     group_entry->comm, &status);

            memcpy(data, l_entry->data, member_entry->datatype_size * member_entry->current_count); 

            if (group_entry->current_rank == 2 && member_id == 90) {
               printf("\nFENIX RESTORE:\n\nCurrent rank %d needs restoration.\n"
                      "Recovering data from rank %d. Data received is %d.\n",
                      group_entry->current_rank, group_entry->out_rank, 
                      ((int *)l_entry->data)[0]);
            }

         } else if (member_entry->main_flag == 0 && member_entry->backup_flag == 1) {
            /* backup commit */

            MPI_Recv(b_l_entry->data, member_entry->datatype_size * member_entry->current_count, 
                     MPI_BYTE, group_entry->out_rank, 0,
                     group_entry->comm, &status);
         
            memcpy(data, b_l_entry->data, member_entry->datatype_size * member_entry->current_count);

            if (group_entry->current_rank == 2 && member_id == 90) {
               printf("\nFENIX RESTORE:\n\nCurrent rank %d needs restoration.\n"
                      "Recovering data from rank %d. Data received is %d.\n",
                      group_entry->current_rank, group_entry->out_rank, 
                      ((int *)b_l_entry->data)[0]);
            }
         } 

         member_entry->state = OCCUPIED;
      }

      if (current_state != NEEDFIX) {

         fenix_buffer_entry_t *l_entry;

         if (member_entry->main_flag == 1 && member_entry->backup_flag == 0) {
            l_entry = &(member_entry->local_entry);
         } else if (member_entry->main_flag == 0 && member_entry->backup_flag == 1) {
            l_entry = &(member_entry->backup_local_entry);
         }

         member_entry->user_data = data;
         memcpy(member_entry->user_data, l_entry->data, member_entry->current_count * member_entry->datatype_size);
      }
   }

   return rtn;
}

int __Fenix_Data_commit_barrier(int group_id, int *time_stamp) {

   int rtn, group_index, member_index;

   rtn = 0;

   group_index = __fenix_search_groupid(group_id, fenix.data_recovery);

   if (group_index == -1) {
      fprintf(stderr, "ERROR Fenix_Data_commit_barrier: group_id <%d> does not exist\n",
              group_id);
      rtn = -1;
   } else {
      fenix_group_t *group = fenix.data_recovery;
      fenix_group_entry_t *group_entry = &(group->group_entry[group_index]);
      fenix_member_t *member = group_entry->member;
      
      /* set the flags for each member in the group */
      for (member_index = 0; member_index < member->count; member_index++) {
         fenix_member_entry_t *member_entry = &(member->member_entry[member_index]);
         member_entry->main_flag = 1;
         member_entry->backup_flag = 0;
      }

      MPI_Barrier(group_entry->comm);
   }

   return rtn;
}

int __Fenix_Data_group_delete(int group_id) {
   return 0;
}

int __Fenix_Data_member_delete(int group_id, int member_id) {
   return 0;
}

int __fenix_member_set_attribute(int group_id, int member_id,
                                 int attributename, void *attributevalue,
                                 int *flag) {
   
   int rtn, group_index, member_index;

   group_index = __fenix_search_groupid(group_id, fenix.data_recovery);
   member_index = __fenix_search_member_id(group_index, member_id);

   rtn = 0;

   if (group_index == -1) {
      fprintf(stderr, "ERROR Fenix_Data_member_attr_set: group_id <%d> does not exist\n",
              group_id);
      rtn = -1;
   } else if (member_index == -1) {
      fprintf(stderr, "ERROR Fenix_Data_member_attr_set: member_id <%d> does not exist\n",
              member_id);
      rtn = -1;
   } else if (fenix.role == 0) {
      fprintf(stderr, "ERROR Fenix_Data_member_attr_set: cannot be called on role: <%s> \n",
              "FENIX_ROLE_INITIAL_RANK");
      rtn = -1;
   } else {

      fenix_group_t *group = fenix.data_recovery;
      fenix_group_entry_t *group_entry = &(group->group_entry[group_index]);
      fenix_member_t *member = group_entry->member;
      fenix_member_entry_t *member_entry = &(member->member_entry[member_index]);;

      if (attributename == FENIX_DATA_MEMBER_ATTRIBUTE_BUFFER) {
         member_entry->user_data = attributevalue;
      }

   }

   return rtn;
}

/* Need to move this to another file */
int __fenix_search_member_id(int group_index, int key) {

   int member_ndx, found;

   fenix_group_t *group = fenix.data_recovery;
   fenix_group_entry_t *group_entry = &(group->group_entry[group_index]);
   fenix_member_t *member = group_entry->member;

   found = -1;

   for (member_ndx = 0; (found == -1) && (member_ndx < member->total_size);
        member_ndx++) {
      fenix_member_entry_t *member_entry = &(member->member_entry[member_ndx]);
      if (key == member_entry->member_id) {
         found = member_ndx;
      }
   }
   return found;
}

/* Also need to move this to another file */
int __fenix_find_next_member_position(fenix_member_t *member) {

   int member_ndx, found;

   fenix_member_t *this_member = member;

   found = -1;
  
   for (member_ndx = 0; (found == -1) && (member_ndx < this_member->total_size);
        member_ndx++) {
      fenix_member_entry_t *member_entry = &(this_member->member_entry[member_ndx]);
      if (member_entry->state == EMPTY || member_entry->state == DELETED) {
         found = member_ndx;
      }
   }
   return found;
}

int __fenix_join_group(fenix_group_t *group, fenix_group_entry_t *entry, MPI_Comm comm) {

   int found, ndx;

   fenix_group_t *g = group;
   fenix_group_entry_t *ge = entry;
   
   int current_rank_attr[GROUP_ENTRY_ATTR_SIZE];
   int other_rank_attr[GROUP_ENTRY_ATTR_SIZE];

   current_rank_attr[0] = g->count;
   current_rank_attr[1] = ge->group_id;
   current_rank_attr[2] = ge->timestamp;
   current_rank_attr[3] = ge->state;

   MPI_Allreduce(current_rank_attr, other_rank_attr, GROUP_ENTRY_ATTR_SIZE,
                 MPI_INT, fenix.agree_op, comm);

   found = -1;
   for (ndx = 0; found != 1 && (ndx < GROUP_ENTRY_ATTR_SIZE); ndx++) {
      if (current_rank_attr[ndx] != other_rank_attr[ndx]) {
         switch (ndx) {
            case 0:
               debug_print("ERROR ranks did not agree on g-count: %d\n",
                            current_rank_attr[0]);
               break;
            case 1:
               debug_print("ERROR ranks did not agree on g-group_id: %d\n",
                            current_rank_attr[1]);
               break;
            case 2:
               debug_print("ERROR ranks did not agree on g-timestamp: %d\n",
                            current_rank_attr[2]);
               break;
            case 3:
               debug_print("ERROR ranks did not agree on g-state: %d\n",
                            current_rank_attr[3]);
               break;
            default:
               break;
         }
         found = 1;
      }
   }

   return found;
   //return 0;
}

int __fenix_join_member(fenix_member_t *member, fenix_member_entry_t *entry, MPI_Comm comm) {

   int found, ndx;

   fenix_member_t *m = member;
   fenix_member_entry_t *me = entry;
   
   int current_rank_attr[NUM_MEMBER_ATTR_SIZE];
   int other_rank_attr[NUM_MEMBER_ATTR_SIZE];

   current_rank_attr[0] = m->count;
   current_rank_attr[1] = me->member_id;
   current_rank_attr[2] = me->state;

   MPI_Allreduce(current_rank_attr, other_rank_attr, NUM_MEMBER_ATTR_SIZE,
                 MPI_INT, fenix.agree_op, comm);

   found = -1;
   for (ndx = 0; found != 1 && (ndx < NUM_MEMBER_ATTR_SIZE); ndx++) {
      if (current_rank_attr[ndx] != other_rank_attr[ndx]) {
         switch (ndx) {
            case 0:
               debug_print("ERROR ranks did not agree on m-count: %d\n",
                            current_rank_attr[0]);
               break;
            case 1:
               debug_print("ERROR ranks did not agree on m-member_id: %d\n",
                            current_rank_attr[1]);
               break;
            case 2:
               debug_print("ERROR ranks did not agree on m-state: %d\n",
                            current_rank_attr[2]);
               break;
            default:
               break;
         }
         found = 1;
      }
   }

   return found;
}
