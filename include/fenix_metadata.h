#ifndef FENIX_METADATA_H
#define FENIX_METADATA_H
#include "fenix_data_recovery.h"

#if 1
void __fenix_init_group_metadata ( fenix_group_entry_t *gentry, int groupid, MPI_Comm comm, int timetamp, int depth  );

void __fenix_reinit_group_metadata ( fenix_group_entry_t *gentry  );

void __fenix_init_member_metadata ( fenix_member_entry_t *mentry, int memberid, void *data, int count, MPI_Datatype datatype );

void __fenix_init_member_store_packet( fenix_member_store_packet_t *lentry_packet, fenix_local_entry_t *lentry, int flag );
#endif

#if 0
inline void __fenix_init_group_metadata ( fenix_group_entry_t *gentry, int groupid, MPI_Comm comm, int timestamp,
                                    int depth  )
{
   gentry->groupid = groupid;
   gentry->comm = comm;
   gentry->timestart = timestamp;
   gentry->timestamp = timestamp;
   gentry->depth = depth + 1;
   gentry->state = OCCUPIED;
}

inline void __fenix_reinit_group_metadata ( fenix_group_entry_t *gentry  )

{
  gentry->current_rank = __fenix_get_current_rank( gentry->comm );
  gentry->comm_size    = __fenix_get_world_size( gentry->comm );
  gentry->in_rank      = ( gentry->current_rank + gentry->comm_size - gentry->rank_separation ) % gentry->comm_size;
  gentry->out_rank     = ( gentry->current_rank + gentry->comm_size + gentry->rank_separation ) % gentry->comm_size;
}

inline void __fenix_init_member_metadata ( fenix_member_entry_t *mentry, int memberid, void *data, int count, MPI_Datatype datatype )

{
    mentry->memberid = memberid;
    mentry->state = OCCUPIED;
    mentry->user_data = data;
    mentry->current_count = count;
    mentry->current_datatype = datatype;
    int dsize;
    MPI_Type_size(datatype, &dsize);

    mentry->datatype_size = mentry->current_size = dsize;
}


inline void __fenix_init_member_store_packet ( fenix_member_store_packet_t *lentry_packet, fenix_local_entry_t *lentry, int flag )
{
  if ( flag == 0 ) {
    lentry_packet->rank = lentry->currentrank;
    lentry_packet->datatype = lentry->datatype;
    lentry_packet->entry_count = lentry->count;
    lentry_packet->entry_size  = lentry->datatype_size;
    lentry_packet->entry_real_count  = lentry->count;
    lentry_packet->num_blocks  = 0;
  } else if ( flag == 1 ) {
    lentry_packet->rank = lentry->currentrank;
    lentry_packet->datatype = lentry->datatype;
    lentry_packet->entry_count = lentry->count;
    lentry_packet->entry_size  = lentry->datatype_size;
    lentry_packet->entry_real_count  = 0;
    lentry_packet->num_blocks  = 0;
  } else if (flag == 2 ) { /* Subset */


  }
}
#endif
#endif // FENIX_METADATA_H

