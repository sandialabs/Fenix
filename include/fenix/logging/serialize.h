#ifndef FENIX_LOGGING_SERIALIZE_H
#define FENIX_LOGGING_SERIALIZE_H
#include <cassert>
#include <cstdio>
#include <vector>
#include <set>
#include <type_traits>
#include <concepts>
#include <istream>
#include <ostream>
#include <mpi.h>

namespace fenix::logging::serialize {
// Helpers for understanding container types
template <template <typename...> typename Base, typename T>
struct container_type : std::false_type {
  using type = void;
};
template <template <typename...> typename Base, typename T, typename... U>
struct container_type<Base, Base<T, U...>> : std::true_type {
  using type = T;
};

template <template <typename...> typename Base, typename T>
constexpr bool container_type_v = container_type<Base, T>::value;

template <template <typename...> typename Base, typename T>
using container_type_t = typename container_type<Base, T>::type;

// Helpers for understanding if a type is readable/writeable
template <typename T>
struct writable;
template <typename T>
constexpr bool writable_v = writable<T>::value;

template <typename T>
struct readable;
template <typename T>
constexpr bool readable_v = readable<T>::value;

template <typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

template <template <typename...> typename Base, typename T>
concept SerializableContainer =
  container_type_v<Base, T> && writable_v<container_type_t<Base, T>>;
template <template <typename...> typename Base, typename T>
concept DeserializableContainer =
  container_type_v<Base, T> && readable_v<container_type_t<Base, T>>;

// Convenience functions that convert to the definitions below
template <typename T>
void write(std::ostream& s, const T* t, int n);
template <typename T>
void write(std::ostream& s, const T&& t);

template <typename T>
void read(std::istream& s, T* t, int n);
template <typename T>
T read(std::istream& s);

// Base read/write functions
template <typename T>
concept FunctionSerializable =
  requires(const T t, std::ostream& o) { t.serialize(o); };
template <typename T>
concept Serializable = FunctionSerializable<T> || TriviallyCopyable<T>;
template <Serializable T>
void write(std::ostream& s, const T& t) {
  if constexpr (FunctionSerializable<T>) {
    t.serialize(s);
  } else {
    s.write(static_cast<const char*>(static_cast<const void*>(&t)), sizeof(T));
  }
}

template <typename T>
concept FunctionDeserializable = requires(T t, std::istream& i) {
  T(i);
  t = std::move(t);
};
template <typename T>
concept Deserializable = TriviallyCopyable<T> || FunctionDeserializable<T>;
template <Deserializable T>
void read(std::istream& s, T& t) {
  if constexpr (FunctionDeserializable<T>) {
    t = T(s);
  } else {
    s.read(static_cast<char*>(static_cast<void*>(&t)), sizeof(T));
  }
}

// Vector read/write
template <typename T>
concept SerializableVector = SerializableContainer<std::vector, T>;
template <typename T>
concept DeserializableVector = DeserializableContainer<std::vector, T>;

template <SerializableVector T>
void write(std::ostream& s, const T& t) {
  write<int>(s, t.size());
  write(s, t.data(), t.size());
}
template <DeserializableVector T>
void read(std::istream& s, T& t) {
  t.resize(read<int>(s));
  read(s, t.data(), t.size());
}

// Set read/write
template <typename T>
concept SerializableSet = SerializableContainer<std::set, T>;
template <typename T>
concept DeserializableSet = DeserializableContainer<std::set, T>;

template <SerializableSet T>
void write(std::ostream& s, const T& st) {
  write<int>(s, st.size());
  for (auto& t : st) write(s, t);
}
template <DeserializableSet T>
void read(std::istream& s, T& st) {
  assert(st.empty());
  int size = read<int>(s);
  for (int i = 0; i < size; i++) {
    auto [it, inserted] = st.insert(read<container_type_t<std::set, T>>(s));
    assert(inserted);
  }
}

// Optional read/write
template <typename T>
concept SerializableOptional = SerializableContainer<std::optional, T>;
template <typename T>
concept DeserializableOptional = DeserializableContainer<std::optional, T>;

template <SerializableOptional T>
void write(std::ostream& s, const T& t) {
  write<bool>(s, t);
  if (t) write(s, *t);
}
template <DeserializableOptional T>
void read(std::istream& s, T& t) {
  if (read<bool>(s)) {
    t.emplace(read<container_type_t<std::optional, T>>(s));
  } else {
    t.reset();
  }
}

// MPI_Datatype read/write
void write(std::ostream& s, const MPI_Datatype& d);
void read(std::istream& s, MPI_Datatype& d);

// MPI_Op read/write
void write(std::ostream& s, const MPI_Op& d);
void read(std::istream& s, MPI_Op& d);

// Read/write helper structs and convenience functions
template <typename T>
struct writable {
  static constexpr bool value = Serializable<T> ||
                                std::is_same_v<T, MPI_Datatype> ||
                                std::is_same_v<T, MPI_Op>;
};
template <template <typename...> typename Base, typename T, typename... Args>
struct writable<Base<T, Args...>> {
  static constexpr bool value =
    SerializableVector<T> || SerializableSet<T> || SerializableOptional<T>;
};

template <typename T>
struct readable {
  static constexpr bool value = Deserializable<T> ||
                                std::is_same_v<T, MPI_Datatype> ||
                                std::is_same_v<T, MPI_Op>;
};
template <template <typename...> typename Base, typename T, typename... Args>
struct readable<Base<T, Args...>> {
  static constexpr bool value = DeserializableVector<T> ||
                                DeserializableSet<T> ||
                                DeserializableOptional<T>;
};

template <typename T>
void write(std::ostream& s, const T* t, int n) {
  if constexpr (!FunctionSerializable<T> && TriviallyCopyable<T>) {
    // shortcut for trivial types
    s.write(
      static_cast<const char*>(static_cast<const void*>(t)), sizeof(T) * n
    );
  } else {
    for (int i = 0; i < n; i++) write(s, t[i]);
  }
}
template <typename T>
void write(std::ostream& s, const T&& t) {
  write(s, t);
}

template <typename T>
void read(std::istream& s, T* t, int n) {
  if constexpr (!FunctionDeserializable<T> && TriviallyCopyable<T>) {
    // shortcut for trivial types
    s.read(static_cast<char*>(static_cast<void*>(t)), sizeof(T) * n);
  } else {
    for (int i = 0; i < n; i++) read(s, t[i]);
  }
}
template <typename T>
T read(std::istream& s) {
  if constexpr (FunctionDeserializable<T>) {
    // force copy elision when possible
    return T(s);
  } else {
    T t;
    read(s, t);
    return t;
  }
}

} //namespace fenix::logging::serialize

#endif
