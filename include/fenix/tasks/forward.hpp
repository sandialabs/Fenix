
// Forward declarations to avoid apps needing C++20 unless they're directly
// using the coroutines

#ifndef FENIX_TASKS_FORWARD_HPP
#define FENIX_TASKS_FORWARD_HPP

namespace fenix::tasks {
template <typename T, bool eager = false>
class Task;

template <typename T>
using LazyTask = Task<T, false>;
}

#endif // FENIX_TASKS_FORWARD_HPP
