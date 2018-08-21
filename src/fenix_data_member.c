/*
 *
 *
 *
 */

#include <mpi.h>
#include "fenix_data_member.h"
#include "fenix_data_group.h"
//#include "fenix_data_packet.h"
#include "fenix_ext.h"

fenix_member_t *__fenix_data_member_init() {

   int member_index;   

   fenix_member_t *member = (fenix_member_t *)
                            s_calloc(1, sizeof(fenix_member_t));
   
   member->count = 0;
   member->total_size = __FENIX_DEFAULT_MEMBER_SIZE; //may need to resize 
                                                     //if value goes past max
   member->member_entry = (fenix_member_entry_t *) s_malloc(
           __FENIX_DEFAULT_MEMBER_SIZE * sizeof(fenix_member_entry_t));

   //insert default values into member entries
   //when user adds members, these values are set elsewhere
   for (member_index = 0; member_index < 
                           __FENIX_DEFAULT_MEMBER_SIZE; member_index++) {
      //point to the first member entry in the member allocated above
      fenix_member_entry_t *member_entry = &(member->member_entry[member_index]);
      member_entry->member_id = -1;
      member_entry->state = EMPTY;
  
      /* NEW */
      member_entry->stored = 0;
      member_entry->main_flag = 0;
      member_entry->backup_flag = 0;

      /* NEWER */
      member_entry->remote_main_flag = 0;
      member_entry->remote_backup_flag = 0;

      /* not implementing versioning, however may need some of its functionality */
      //member_entry->version = __fenix_data_version_init(); //not implementing version  
   }
   
   return member;
}

//called as part of fenix_finalize to free the allocated memory
void __fenix_data_member_destroy(fenix_member_t *member) {

   int ndx;
  
   /* Un-comment if versioning is implemented */
   /*for (ndx = 0; ndx < member->total_size; ndx++) {
      __fenix_data_version_destroy(member->member_entry[ndx].version);
   }*/

   free(member->member_entry);
   free(member);
}

void __fenix_ensure_ember_capacity(fenix_member_t *member) {

}

void __fenix_ensure_version_capacity_from_member(fenix_member_t *member) {

}

/*void __fenix_data_member_reinit(fenix_member_t *m, fenix_metadata_packet_t packet, 
                                int state) {
  fenix_member_t *member = m;
  int start_index = member->total_size;
  member->count = 0;
  member->temp_count = packet.count;
  member->total_size = packet.total_size;
  member->member_entry = (fenix_member_entry_t *) s_realloc(member->member_entry,
                                                            (member->total_size) *
                                                            sizeof(fenix_member_entry_t));
  if (__fenix_options.verbose == 50) {
    verbose_print("c-rank: %d, role: %d, m-count: %d, m-size: %d\n",
                    __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role,
                  member->count, member->total_size);
  }

  int member_index;
   Why start_index is set to the number of member entries? 
  for (member_index = 0; member_index < member->total_size; member_index++) {
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    mentry->member_id = -1;
    mentry->state = OCCUPIED;
    if (__fenix_options.verbose == 50) {
      verbose_print("c-rank: %d, role: %d, m-memberid: %d, m-state: %d\n",
                      __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role,
                    mentry->member_id, mentry->state);
    }

    //mentry->version = __fenix_data_version_init();
  }
}*/

/*int __fenix_search_member_id(int group_index, int key) {

   //fenix_group_t *group = __fenix_g_data_recovery;

}*/
