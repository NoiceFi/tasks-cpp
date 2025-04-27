#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <array>
#include <iostream>
#include <stack>
#include <vector>
#include <type_traits>
#include <concepts>
#include <cassert>
const size_t DYNAMIC_CAPACITY = std::numeric_limits<std::size_t>::max();

template <typename T, size_t Capacity>
struct Fields {
  size_t size = 0;
  char buffer_begin[(Capacity + 1) * sizeof(T)];
  T* begin = nullptr;
};
template <typename T>
struct Fields<T, DYNAMIC_CAPACITY> {
  size_t size = 0;
  size_t cap = 0;
  T* buffer_begin = nullptr;
  T* begin = nullptr;
};
template <typename T, size_t Capacity>
struct iter_fields {
  T* begin = nullptr;
  T* buffer_begin = nullptr;
  T* ptr = nullptr;
};
template <typename T>
struct iter_fields <T, DYNAMIC_CAPACITY> {
  size_t size = 0;
  T* begin = nullptr;
  T* buffer_begin = nullptr;
  T* ptr = nullptr;
};
template <typename T, size_t Capacity = DYNAMIC_CAPACITY>
struct CircularBuffer {
  Fields<T, Capacity> a;

  size_t cap() const {
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      return a.cap;
    }
    else {
      return Capacity;
    }
  }
  size_t size() const {
    return a.size;
  }
  size_t capacity() const {
    return cap();
  }
  bool full() const {
    return size() == capacity();
  }
  template<bool isConst>
  struct base_iterator {
    using pointer = std::conditional_t<isConst, const T*, T*>;
    using reference = std::conditional_t<isConst, const T&, T&>;
    using value_type = std::conditional_t<isConst, const T, T>;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;
    iter_fields<T, Capacity> iter;
    int size() const {
      if constexpr (Capacity == DYNAMIC_CAPACITY) {
        return iter.size;
      }
      return Capacity + 1;
    }
    base_iterator(T* _ptr, T* _begin, T* _buffer_begin, size_t _cap) {
      static_assert(Capacity == DYNAMIC_CAPACITY);
      iter.ptr = _ptr;
      iter.begin = _begin;
      iter.size = _cap + 1;
      iter.buffer_begin = _buffer_begin;
    }
    base_iterator(T* _ptr, T* _begin, T* _buffer_begin) {
      static_assert(Capacity != DYNAMIC_CAPACITY);
      iter.ptr = _ptr;
      iter.begin = _begin;
      iter.buffer_begin = _buffer_begin;
    }
    base_iterator(const base_iterator& other) {
      iter.ptr = other.iter.ptr;
      iter.begin = other.iter.begin;
      iter.buffer_begin = other.iter.buffer_begin;
      if constexpr (Capacity == DYNAMIC_CAPACITY) {
        iter.size = other.iter.size;
      }
    }
    base_iterator operator=(const base_iterator& other) {
      iter.ptr = other.iter.ptr;
      iter.begin = other.iter.begin;
      iter.buffer_begin = other.iter.buffer_begin;
      if constexpr (Capacity == DYNAMIC_CAPACITY) {
        iter.size = other.iter.size;
      }
      return *this;
    }
    base_iterator& operator+=(size_t difference) {
      iter.ptr = iter.buffer_begin + ((iter.ptr - iter.buffer_begin) + difference) % size();
      return *this;
    }
    base_iterator& operator-=(size_t difference) {
      (*this) += (size() - difference);
      return *this;
    }
    base_iterator operator+(size_t difference) const {
      base_iterator tmp = *this;
      tmp += difference;
      return tmp;
    }
    base_iterator operator-(size_t difference) const {
      return operator+(size() - difference);
    }
    base_iterator& operator++() {
      *this += 1;
      return (*this);
    }
    base_iterator operator++(int) {
      auto copy = (*this);
      (*this) += 1;
      return copy;
    }
    base_iterator& operator--() {
      *this -= 1;
      return (*this);
    }
    base_iterator operator--(int) {
      auto copy = (*this);
      (*this) -= 1;
      return copy;
    }
    pointer operator->() const {
      return iter.ptr;
    }
    reference operator*() const {
      return *iter.ptr;
    }
    difference_type operator-(const base_iterator& other) const {
      size_t dif1 = (iter.ptr - iter.begin) >= 0 ? iter.ptr - iter.begin :
        size() - (iter.begin - iter.ptr);
      size_t dif2 = (other.iter.ptr - other.iter.begin) >= 0 ? other.iter.ptr - other.iter.begin :
        size() - (other.iter.begin - other.iter.ptr);
      return dif1 - dif2;
    }
    bool operator==(const base_iterator& other) const {
      return iter.ptr == other.iter.ptr;
    }
    bool operator!=(const base_iterator& other) const {
      return !(iter.ptr == other.iter.ptr);
    }
    bool operator<(const base_iterator& other) const {
      return (*this - other) < 0;
    }
    bool operator>(const base_iterator& other) const {
      return other < *this;
    }
    bool operator<=(const base_iterator& other) const {
      return !(*this > other);
    }
    bool operator>=(const base_iterator& other) const {
      return !(*this < other);
    }
    
  };
  template<bool is_const>
  friend base_iterator<is_const> operator+(size_t difference, const base_iterator<is_const>& it) {
    return it + difference;
  }
  using iterator = base_iterator<false>;
  using const_iterator = base_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() {
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      return iterator(a.begin, a.begin, a.buffer_begin, cap());
    }
    else {
      return iterator(a.begin, a.begin, reinterpret_cast<T*>(a.buffer_begin));
    }

  }
  const_iterator cbegin() const {
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      return const_iterator(a.begin, a.begin, a.buffer_begin, cap());
    }
    else {
      return const_iterator(a.begin, a.begin, reinterpret_cast<T*>(const_cast<char*>(a.buffer_begin)));
    }
  }
  const_iterator begin() const {
    return cbegin();
  }
  iterator end() {
    return begin() + a.size;
  }
  const_iterator end() const {
    return cend();
  }
  const_iterator cend() const {
    return cbegin() + a.size;
  }
  reverse_iterator rbegin() {
    return reverse_iterator(end());
  }
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(cbegin());
  }
  const_reverse_iterator rbegin() const {
    return crbegin();
  }
  reverse_iterator rend() {
    return reverse_iterator(begin());
  }
  const_reverse_iterator crend() const {
    return const_reverse_iterator(cbegin());
  }
  const_reverse_iterator rend() const {
    return crend();
  }

  bool empty() const {
    return size() == 0;
  }
  T& operator[](size_t id) {
    return *(begin() + id);
  }
  const T& operator[](size_t id) const {
    return *(cbegin() + id);
  }
  T& at(size_t id) {
    if (id >= size()) {
      throw std::out_of_range("id >= size");
    }
    return operator[](id);
  }

  void change_capacity(size_t new_cap) {
    static_assert(Capacity == DYNAMIC_CAPACITY);
    T* new_buffer = reinterpret_cast<T*>(new char[(new_cap + 1) * sizeof(T)]);
    size_t cnt = 0;
    try {
      for (auto it = begin(); it != end(); ++it) {
        new(new_buffer + cnt) T(*(it));
        cnt++;
      }
    }
    catch (...) {
      for (size_t i = 0; i < cnt; i++) {
        (new_buffer + i)->~T();
      }
      delete[] reinterpret_cast<char*>(new_cap);
      throw;
    }
    for (auto it = begin(); it != end(); ++it) {
      (it.iter.ptr)->~T();
    }
    delete[] reinterpret_cast<char*> (a.buffer_begin);
    a.buffer_begin = new_buffer;
    a.cap = new_cap;
  }

  CircularBuffer() {
    static_assert(Capacity != DYNAMIC_CAPACITY);
    a.begin = reinterpret_cast<T*>(a.buffer_begin);
    a.size = 0;
  }
  CircularBuffer(size_t cap) {
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      a.buffer_begin = reinterpret_cast<T*>(new char[(cap + 1) * sizeof(T)]);
      /*try {
        for (; id < cap + 1; id++) {
          new(a.buffer_begin + id) T();
        }
      }
      catch (...) {
        for (int i = 0; i < id; i++) {
          (a.buffer_begin + i)->~T();
        }
        throw;
      }*/
      a.begin = a.buffer_begin;
      a.size = 0;
      a.cap = cap;
    }
    else {
      if (Capacity != cap) {
        throw std::invalid_argument("Capacity_error");
      }
      a.begin = reinterpret_cast<T*> (a.buffer_begin);
      a.size = 0;
    }
  }
  CircularBuffer(const CircularBuffer& other) {
    if constexpr (Capacity == DYNAMIC_CAPACITY) a.buffer_begin = reinterpret_cast<T*>(new char[(other.cap() + 1) * sizeof(T)]);
    size_t cnt = 0;
    a.begin = reinterpret_cast<T*> (a.buffer_begin);
    try {
      for (auto it = other.begin(); it != other.end(); ++it) {
        new(a.begin + cnt) T(*(it));
        cnt++;
      }
    }
    catch (...) {
      for (size_t i = 0; i < cnt; i++) {
        (a.begin + i)->~T();
      }
      if (Capacity == DYNAMIC_CAPACITY) delete[] reinterpret_cast<char*>(a.buffer_begin);
      throw;
    }
    a.size = other.size();
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      a.cap = other.cap();
    }
  }
  void swap(CircularBuffer& other) {
    std::swap(a.size, other.a.size);
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      std::swap(a.cap, other.a.cap);
      std::swap(a.begin, other.a.begin);
      std::swap(a.buffer_begin, other.a.buffer_begin);
    }
    else {
      std::swap(a.buffer_begin, other.a.buffer_begin);
    }
  }
  CircularBuffer& operator= (CircularBuffer& other) {
    CircularBuffer<T, Capacity> copy(other);
    swap(copy);
    return *this;
  }
  ~CircularBuffer() {
    for (auto it = begin(); it != end(); it++) {
      (it.iter.ptr)->~T();
    }
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      delete[] reinterpret_cast<char*>(a.buffer_begin);
    }
  }
  void push_front(const T& object) {
    auto it = begin();
    it--;
    if (a.size == cap()) {
      ((it - 1).iter.ptr)->~T();
    }
    new (it.iter.ptr) T(object);
    a.begin = it.iter.ptr;
    if (a.size != cap()) a.size++;
  }
  void push_back(const T& object) {
    auto it = begin() + a.size;
    new (it.iter.ptr) T(object);
    if (a.size == cap()) {
      a.begin->~T();
      a.begin++;
    }
    if (a.size != cap()) a.size++;
  }
  void pop_back() {
    auto it = begin() + a.size - 1;
    (it.iter.ptr)->~T();
    a.size--;
  }
  void pop_front() {
    auto it = begin();
    (it.iter.ptr)->~T();
    a.size--;
    a.begin = (++it).iter.ptr;
  }
  void insert(iterator it, const T& obj) {
    if (it == end()) {
      push_back(obj);
      return;
    }
    if (it == begin() && size() == cap()) {
      return;
    }
    if (!empty()) {
      for (auto iter = end() - 1; iter != (it - 1); iter--) {
        if (iter + 1 == end() && cap() == size()) {
          iterator j = begin();
           j.iter.ptr->~T();
        }
        new ((iter + 1).iter.ptr) T(*iter);
        (iter.iter.ptr)->~T();
      }
    }
    new (it.iter.ptr) T(obj);
    if (size() != cap()) {
      a.size++;
    } else {
      a.begin++;
    }
  }
  void erase(iterator it) {
    for (auto iter = it; iter != end() - 1; iter++) {
      (iter.iter.ptr)->~T();
      new (iter.iter.ptr) T(*(iter + 1));
    }
    ((end() - 1).iter.ptr)->~T();
    a.size--;
  }
};
