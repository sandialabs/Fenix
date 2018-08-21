/*
 *
 *
 *
 */

#include <mpi.h>
#include "fenix_data_group.h"
#include "fenix_data_member.h"
#include "fenix_ext.h"
//#include "fenix_data_packet.h"
#include "fenix-config.h"

/* Initialize the group entries, and call the function that
 * initialies the member entries */
fenix_group_t * __fenix_data_group_init() {

   int group_index;

   /* allocate the data_group that will hold all of the
    * group members 
    */

   //fprintf(stderr, "Fenix group size: %d\n", sizeof(fenix_group_t));
   fenix_group_t *group = (fenix_group_t *)
                          s_calloc(1, sizeof(fenix_group_t));

   group->count = 0;   //because there are no group members yet
   group->total_size = __FENIX_DEFAULT_GROUP_SIZE; //32 members allowed at first

   /* allocate sufficient space for 32 group entries */
   group->group_entry = (fenix_group_entry_t *) s_malloc
                        (__FENIX_DEFAULT_GROUP_SIZE * sizeof(fenix_group_entry_t));

   /* initialize all of the group entries */
   for (group_index = 0; group_index < __FENIX_DEFAULT_GROUP_SIZE; group_index++) {
      fenix_group_entry_t *group_entry = &(group->group_entry[group_index]);

      //group_entry->depth = 0;  //likely to be removed, no versioning
      group_entry->group_id = -1;   //group entry receives a value when user creates it
      group_entry->timestamp = 0;   //not sure what timestamps are used for yet
      group_entry->state = EMPTY;   //empty until this entry is created by user
   
      /* set buffer flags to 0 */
      //group_entry->main_flag = 0;
      //group_entry->backup_flag = 0;

      /* call function to initialize data members for each group entry */
      group_entry->member = __fenix_data_member_init(); 
   }

   return group;
}

void __fenix_data_group_destroy(fenix_group_t *group) {

   int ndx;

   for (ndx = 0; ndx < group->total_size; ndx++) {
      __fenix_data_member_destroy(group->group_entry[ndx].member);
   }

   free(group->group_entry);
   free(group);
}

void __fenix_ensure_group_capacity(fenix_group_t *group) {

}

/* Return the key if a group entry with the same group_id exists */
int __fenix_search_groupid(int key, fenix_group_t *group) {

   int ndx, found;
   
   found = -1;
   for (ndx = 0; (found == -1) && (ndx < group->total_size); ndx++) {
      fenix_group_entry_t *group_entry = &(group->group_entry[ndx]);
      if (key == group_entry->group_id) {
         found = ndx;  //ensure group_index == group_id
      }
   }

   return found;
}

/* Find and return the position of the next available group entry */
int __fenix_find_next_group_position(fenix_group_t *group) {

   int ndx, found;
 
   found = -1;
   for (ndx = 0; (found == -1) && (ndx < group->total_size); ndx++) {
      fenix_group_entry_t *group_entry = &(group->group_entry[ndx]);
      if (group_entry->state == EMPTY || group_entry->state == DELETED) {
         found = ndx;
      }
   }

   return found;
}

/*void __fenix_data_group_reinit(fenix_group_t *g, fenix_metadata_packet_t packet) {
  fenix_group_t *group = g;
  int start_index = group->total_size;
  group->count = packet.count;
  group->total_size = packet.total_size;
  group->group_entry = (fenix_group_entry_t *) s_realloc(group->group_entry,
                                                         (group->total_size) *
                                                         sizeof(fenix_group_entry_t));

  if (__fenix_options.verbose == 48) {
    verbose_print("c-rank: %d, role: %d, g-size: %d\n",
                    __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role, group->total_size);
  }

  int group_index;
  for (group_index = start_index; group_index < group->total_size; group_index++) {
    fenix_group_entry_t *gentry = &(group->group_entry[group_index]);
    //gentry->depth = 1;
    gentry->group_id = -1;
    gentry->timestamp = 0;
    gentry->state = EMPTY;

    if (__fenix_options.verbose == 48) {
      verbose_print(
              "c-rank: %d, role: %d, g-groupid: %d, g-timestamp: %d, g-state: %d\n",
                __fenix_get_current_rank(*__fenix_g_new_world), __fenix_g_role,
              gentry->group_id, gentry->timestamp, gentry->state);
    }

    gentry->member = __fenix_data_member_init();
  }
}*/
