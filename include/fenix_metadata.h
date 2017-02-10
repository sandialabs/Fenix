#ifndef FENIX_METADATA_H
#define FENIX_METADATA_H
#include "fenix_data_recovery.h"

inline void __fenix_init_group_metadata ( fenix_group_entry_t *gentry, MPI_Comm comm, int timetamp, int depth  );

inline void __fenix_reinit_group_metadata ( fenix_group_entry_t *gentry  );

inline void __fenix_init_member_metadata ( fenix_member_entry_t *mentry, void *data, int count, MPI_Datatype datatype );

inline void __fenix_init_member_store_packet_init ( fenix_member_store_packet_t lentry_packet, fenix_local_entry_t lentry, int flag );


#endif // FENIX_METADATA_H

