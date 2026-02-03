
// Forward declarations to avoid apps needing C++20 unless they're directly
// using the coroutines

#ifndef FENIX_TASKS_FORWARD_H
#define FENIX_TASKS_FORWARD_H

namespace fenix::tasks {
template <typename T, bool eager = true>
class Task;

template <typename T>
using LazyTask = Task<T, false>;

class Status;

namespace mpi {
using MPITask = Task<Status>;
}
}

#endif // FENIX_TASKS_FORWARD_H
