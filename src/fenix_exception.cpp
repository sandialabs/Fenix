#include "fenix_exception.hpp"
#include "fenix.h"

namespace fenix {

int register_exception_callback(){
    return Fenix_Callback_register(
      [](MPI_Comm repaired_comm, int fen_err){
        throw CommException(repaired_comm, fen_err);
      }
    );
}

} // namespace fenix
