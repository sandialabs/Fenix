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
template <typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;
template <typename T>
concept Serializable = requires(const T t, std::ostream& o) { t.serialize(o); };
template <typename T>
concept Writable = TriviallyCopyable<T> || Serializable<T>;
template <typename T>
concept Deserializable = requires(T t, std::istream& i) {
  T(i);
  t = std::move(t);
};
template <typename T>
concept Readable = TriviallyCopyable<T> || Deserializable<T>;

template <Writable T>
void write(std::ostream& s, const T* t, int n) {
  if constexpr (Serializable<T>) {
    for (int i = 0; i < n; i++) t[i].serialize(s);
  } else {
    s.write((char*)t, sizeof(T) * n);
  }
}

template <Writable T>
void write(std::ostream& s, const T& t) {
  write(s, &t, 1);
}
void write(std::ostream& s, const MPI_Datatype& d);
void write(std::ostream& s, const MPI_Op& d);
template <Writable T>
void write(std::ostream& s, const T&& t) {
  write(s, &t, 1);
}
template <Writable T>
void write(std::ostream& s, const std::vector<T>& t) {
  write<int>(s, t.size());
  write(s, t.data(), t.size());
}
template <Writable T, typename U>
void write(std::ostream& s, const std::set<T, U>& st) {
  write<int>(s, st.size());
  for (auto& t : st) write(s, t);
}

template <Readable T>
void read(std::istream& s, T* t, int n) {
  if constexpr (Serializable<T>) {
    for (int i = 0; i < n; i++) t[i] = T(s);
  } else {
    s.read((char*)t, sizeof(T) * n);
  }
}
template <Readable T>
T read(std::istream& s) {
  if constexpr (Serializable<T>) {
    return T(s);
  } else {
    T t;
    read(s, &t, 1);
    return t;
  }
}
template <Readable T>
void read(std::istream& s, T& t) {
  read(s, &t, 1);
}
void read(std::istream& s, MPI_Datatype& d);
void read(std::istream& s, MPI_Op& d);
template <Readable T>
void read(std::istream& s, std::vector<T>& t) {
  t.resize(read<int>(s));
  read(s, t.data(), t.size());
}
template <Readable T, typename U>
void read(std::istream& s, std::set<T, U>& st) {
  assert(st.empty());
  int size = read<int>(s);
  for (int i = 0; i < size; i++) {
    auto [it, inserted] = st.insert(read<T>(s));
    assert(inserted);
  }
}

template <typename E, typename T, std::size_t I>
struct TupleIndexCheck {
  static constexpr bool value = std::is_same_v<E, std::tuple_element_t<I, T>>;
  static constexpr std::size_t index = I;
};
template <typename E, typename T, std::size_t... I>
constexpr auto tuple_indexer(std::index_sequence<I...>) {
  return std::disjunction<TupleIndexCheck<E, T, I>...>::index;
}
template <typename Element, typename Tuple>
constexpr std::size_t TupleIndex = tuple_indexer<Element, Tuple>(
  std::make_index_sequence<std::tuple_size_v<Tuple>>{}
);

} //namespace fenix::logging::serialize

#endif
