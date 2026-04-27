#include <concepts>
#include <iostream>
#include <type_traits>

template <typename Target, typename... Types>
struct count_types {
  static const int value = 0;
};

template <typename U, typename Head, typename... Tail>
struct count_types<U, Head, Tail...> {
  static const int value =
    std::is_same_v<U, Head> +count_types<U, Tail...>::value;
};



template <typename... Tail>
class Tuple;

template <>
class Tuple<> {};

template <typename T>
struct isTuple : std::false_type {};

template<typename... Types>
struct isTuple<Tuple<Types...>> : std::true_type {};

template <typename Head, typename... Tail>
class Tuple<Head, Tail...> {

private:

  Head head;
  [[no_unique_address]] Tuple<Tail...> tail;

  template <typename UHead, typename... Tuples>
  explicit Tuple(UHead&& first, Tuples &&...args)
    requires (isTuple<std::decay_t<UHead>>::value && (isTuple<std::decay_t<Tuples>>::value && ...))
  : head(get<0>(std::forward<UHead>(first))),
    tail(std::forward<UHead>(first).tail,
      std::forward<Tuples>(args)...) {}

  template <typename UHead, typename... Tuples>
  explicit Tuple(Tuple<>, UHead&& first, Tuples &&...args)
    requires (isTuple<std::decay_t<UHead>>::value && (isTuple<std::decay_t<Tuples>>::value && ...))
  : head(get<0>(std::forward<UHead>(first))),
    tail(std::forward<UHead>(first).tail,
      std::forward<Tuples>(args)...) {}
public:
  explicit(
    !(requires (Head a) { a = {}; })
    || (!(requires(Tail b) { b = {}; }) || ...)
  )
    Tuple()
    requires(
  std::is_default_constructible_v<Head> &&
    (std::is_default_constructible_v<Tail> && ...)
    )
    : head(), tail() {}

  explicit(
    !std::is_convertible_v<const Head&, Head> &&
    (!std::is_convertible_v<const Tail&, Tail> && ...)
    )
    Tuple(const Head& head, const Tail&... tail)
    requires(
  std::is_copy_constructible_v<Head> &&
    (std::is_copy_constructible_v<Tail> && ...)
    )
    : head(head), tail(tail...) {}

  template <size_t N, typename... Types>
  friend decltype(auto) get(Tuple<Types...>&&);

  template <size_t N, typename... Types>
  friend decltype(auto) get(const Tuple<Types...>&&);

  template <size_t N, typename... Types>
  friend decltype(auto) get(const Tuple<Types...>&);

  template <size_t N, typename... Types>
  friend decltype(auto) get(Tuple<Types...>&);

  template <typename U, typename... Types>
    requires(count_types<U, Types...>::value == 1)
  friend decltype(auto) get(Tuple<Types...>&&);

  template <typename U, typename... Types>
    requires(count_types<U, Types...>::value == 1)
  friend decltype(auto) get(const Tuple<Types...>&&);

  template <typename U, typename... Types>
    requires(count_types<U, Types...>::value == 1)
  friend decltype(auto) get(const Tuple<Types...>&);

  template <typename U, typename... Types>
    requires(count_types<U, Types...>::value == 1)
  friend decltype(auto) get(Tuple<Types...>&);

  template <typename UHead, typename... UTail>
  explicit(
    !std::is_convertible_v<Head, UHead> ||
    (!std::is_convertible_v<Tail, UTail> || ...)
    )
    Tuple(UHead&& head, UTail&&... args)
    requires(
  (sizeof...(Tail) == sizeof...(UTail)) &&
    (std::is_constructible_v<Head, UHead>) &&
    (std::is_constructible_v<Tail, UTail> && ...)
    )
    : head(std::forward<UHead>(head)), tail(std::forward<UTail>(args)...) {}

  template <typename...>
  friend class Tuple;

  template <typename Uhead, typename... UTail>
  explicit(
    (!std::is_convertible_v<const Uhead&, Head>) ||
    (!std::is_convertible_v<const UTail&, Tail> || ...)
    )
    Tuple(const Tuple<Uhead, UTail...>& other)
    requires(
  sizeof...(Tail) == sizeof...(UTail) &&
    (std::is_constructible_v<Head, const Uhead&>) &&
    (std::is_constructible_v<Tail, const UTail&> && ...) &&
    (sizeof...(Tail) != 0 ||
      (!std::is_constructible_v<Head, decltype(other)> &&
        !std::is_constructible_v<decltype(other), Head> &&
        !std::is_same_v<Head, decltype(other.head)>))
    )
    : head(get<0>(other)),
    tail(other.tail) {}

  template <typename Uhead, typename... UTail>
  explicit(
    (!std::is_convertible_v<Uhead&&, Head>) ||
    (!std::is_convertible_v<UTail&&, Tail> || ...)
    )
    Tuple(Tuple<Uhead, UTail...>&& other)
    requires(
  sizeof...(Tail) == sizeof...(UTail) &&
    (std::is_constructible_v<Head, Uhead&&>) &&
    (std::is_constructible_v<Tail, UTail&&> && ...) &&
    (sizeof...(Tail) != 0 ||
      (!std::is_constructible_v<Head, decltype(other)> &&
        !std::is_constructible_v<decltype(other), Head> &&
        !std::is_same_v<Head, decltype(other.head)>))
    )
    : head(get<0>(std::forward<decltype(other)>(other))),
    tail(std::move(other.tail)) {}

  template <typename T1, typename T2>
  Tuple(const std::pair<T1, T2>& other)
    requires(
  (sizeof...(Tail)) == 1 &&
    std::is_constructible_v<Head, decltype(std::get<0>(other))> &&
    (std::is_constructible_v<Tail, decltype(std::get<1>(other))> && ...)
    )
    : head(other.first), tail(other.second) {}

  template <typename T1, typename T2>
  Tuple(std::pair<T1, T2>&& other)
    requires(
  (sizeof...(Tail)) == 1 &&
    std::is_constructible_v<Head,
    decltype(std::get<0>(std::move(other)))> &&
    (std::is_constructible_v<Tail, decltype(std::get<1>(std::move(other)))> && ...)
    )
    : head(std::move(other.first)), tail(std::move(other.second)) {}

  Tuple& operator=(const Tuple& other)
    requires(
  std::is_copy_assignable_v<Head> &&
    (std::is_copy_assignable_v<Tail> && ...)
    )
  {
    head = other.head;
    tail = other.tail;
    return *this;
  }

  Tuple& operator=(Tuple&& other)
    requires(
  std::is_move_assignable_v<Head> &&
    (std::is_move_assignable_v<Tail> && ...)
    )
  {
    head = std::forward<Head>(get<0>(std::move(other)));
    tail = std::move(other.tail);
    return *this;
  }

  Tuple(const Tuple& other)
    requires(
  std::is_copy_constructible_v<Head> &&
    (std::is_copy_constructible_v<Tail> && ...)
    )
    : head(other.head), tail(other.tail) {}

  Tuple(Tuple&& other)
    requires(
  std::is_move_constructible_v<Head> &&
    (std::is_move_constructible_v<Tail> && ...)
    )
    : head(std::move(other.head)), tail(std::move(other.tail)) {}

  template <typename UHead, typename... UTail>
  Tuple& operator=(const Tuple<UHead, UTail...>& other)
    requires(
  (sizeof...(Tail) == sizeof...(UTail)) &&
    (std::is_assignable_v<Head&, const UHead&>) &&
    (std::is_assignable_v<Tail&, const UTail&> && ...)
    )
  {
    head = other.head;
    tail = other.tail;
    return *this;
  }

  template <typename UHead, typename... UTail>
  Tuple& operator=(Tuple<UHead, UTail...>&& other)
    requires(
  (sizeof...(Tail) == sizeof...(UTail)) &&
    (std::is_assignable_v<Head&, UHead>) &&
    (std::is_assignable_v<Tail&, UTail> && ...)
    )
  {
    head = get<0>(std::move(other));
    tail = std::move(other.tail);
    return *this;
  }

  template<typename... UTypes>
  bool operator<(const Tuple<UTypes...>& other) const {
    if constexpr (sizeof...(UTypes) == 0) {
      return false;
    } else {
      if (head != other.head) {
        return head < other.head;
      }
      if constexpr (sizeof...(Tail) == 0) {
        return sizeof...(UTypes) > 1;
      } else {
        return tail < other.tail;
      }
    }
  }

  template<typename... UTypes>
  bool operator==(const Tuple<UTypes...>& other) const {
    if constexpr (sizeof...(UTypes) == 0) {
      return false;
    } else {
      if (head != other.head) {
        return false;
      }
      if constexpr (sizeof...(Tail) == 0) {
        return sizeof...(UTypes) == 1;
      } else {
        return tail == other.tail;
      }
    }
  }

  template<typename... UTypes>
  bool operator>(const Tuple<UTypes...>& other) const {
    return other < (*this);
  }

  template<typename... UTypes>
  bool operator!=(const Tuple<UTypes...>& other) const {
    return !operator==(other);
  }

  template<typename... UTypes>
  bool operator<=(const Tuple<UTypes...>& other) const {
    return !operator>(other);
  }

  template<typename... UTypes>
  bool operator>=(const Tuple<UTypes...>& other) const {
    return !operator<(other);
  }

  template <typename... Tuples>
  friend auto tupleCat(Tuples&&...);

};

template <typename T1, typename T2>
Tuple(const std::pair<T1, T2>) -> Tuple<T1, T2>;

template <typename T1, typename T2>
Tuple(std::pair<T1, T2>&&) -> Tuple<T1, T2>;

template <size_t N, typename... Types>
decltype(auto) get(Tuple<Types...>&& t) {
  if constexpr (N == 0) {
    return std::forward<decltype(t.head)>(t.head);
  } else {
    return get<N - 1>(std::forward<decltype(t.tail)>(t.tail));
  }
}

template <size_t N, typename... Types>
decltype(auto) get(const Tuple<Types...>&& t) {
  if constexpr (N == 0) {
    return std::forward<decltype(t.head)>(t.head);
  } else {
    return get<N - 1>(std::forward<decltype(t.tail)>(t.tail));
  }
}

template <size_t N, typename... Types>
decltype(auto) get(Tuple<Types...>& t) {
  if constexpr (N == 0) {
    return (t.head);
  } else {
    return get<N - 1>(t.tail);
  }
}

template <size_t N, typename... Types>
decltype(auto) get(const Tuple<Types...>& t) {
  if constexpr (N == 0) {
    return (t.head);
  } else {
    return get<N - 1>(t.tail);
  }
}

template <typename U, typename... Types>
  requires(count_types<U, Types...>::value == 1)
decltype(auto) get(Tuple<Types...>&& t) {
  if constexpr (std::is_same_v<decltype(t.head), U>) {
    return std::forward<decltype(t.head)>(t.head);
  } else {
    return get<U>(std::forward<decltype(t.tail)>(t.tail));
  }
}

template <typename U, typename... Types>
  requires(count_types<U, Types...>::value == 1)
decltype(auto) get(Tuple<Types...>& t) {
  if constexpr (std::is_same_v<decltype(t.head), U>) {
    return (t.head);
  } else {
    return get<U>(t.tail);
  }
}

template <typename U, typename... Types>
  requires(count_types<U, Types...>::value == 1)
decltype(auto) get(const Tuple<Types...>& t) {
  if constexpr (std::is_same_v<decltype(t.head), U>) {
    return (t.head);
  } else {
    return get<U>(t.tail);
  }
}

template <typename U, typename... Types>
  requires(count_types<U, Types...>::value == 1)
decltype(auto) get(const Tuple<Types...>&& t) {
  if constexpr (std::is_same_v<decltype(t.head), U>) {
    return std::forward<const decltype(t.head)>(t.head);
  } else {
    return get<U>(std::move(t.tail));
  }
}

template <typename... Args>
auto makeTuple(Args&&... args) {
  return Tuple<std::decay_t<Args>...>(std::forward<Args>(args)...);
}

template <typename... Tuples>
struct CatTupleTypes;

template <>
struct CatTupleTypes<> {
  using type = Tuple<>;
};

template <typename... Types>
struct CatTupleTypes<Tuple<Types...>> {
  using type = Tuple<Types...>;
};

template <typename... Types, typename... UTypes, typename... Other>
struct CatTupleTypes<Tuple<Types...>, Tuple<UTypes...>, Other...> {
  using type =
    typename CatTupleTypes<Tuple<Types..., UTypes...>, Other...>::type;
};

template <typename... Tuples>
auto tupleCat(Tuples &&...tuples) {
  return typename CatTupleTypes<typename std::decay_t<Tuples>...>::type(
    std::forward<Tuples>(tuples)...);
}

template<typename... Args>
Tuple<Args&...> tie(Args... args) {
  return Tuple<Args&...>{ args... };
}

template<typename... Args>
Tuple<Args&&...> forwardAsTuple(Args&&... args) {
  return Tuple<Args&&...>{ std::forward<Args>(args)... };
}
