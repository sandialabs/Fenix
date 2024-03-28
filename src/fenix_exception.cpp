#include "fenix_exception.hpp"
#include "fenix.h"

namespace Fenix {
int register_exception_callback(void* user_data){
    return Fenix_Callback_register(
      [](MPI_Comm repaired_comm, int fen_err, void* data){ 
        throw CommException(repaired_comm, fen_err, data);
      }, 
      user_data
    );
}
}
