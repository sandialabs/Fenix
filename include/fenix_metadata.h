#ifndef FENIX_METADATA_H
#define FENIX_METADATA_H
#include "fenix_data_recovery.h"

void __fenix_init_group_metadata ( fenix_group_entry_t *gentry, MPI_Comm comm, int timetamp,
                                   int depth  );

inline void __fenix_reinit_group_metadata ( fenix_group_entry_t *gentry  );
#endif // FENIX_METADATA_H

