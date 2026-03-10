
// Forward declarations to avoid apps needing C++20 unless they're directly
// using the coroutines

#ifndef FENIX_TASKS_FORWARD_HPP
#define FENIX_TASKS_FORWARD_HPP

namespace fenix::util {
class Status;
}

namespace fenix::tasks {
template <typename T, bool eager = false>
class Task;

template <typename T>
using LazyTask = Task<T, false>;

using util::Status;
}

namespace fenix::tasks::mpi {
using MPITask = Task<Status>;
}

#endif // FENIX_TASKS_FORWARD_HPP
