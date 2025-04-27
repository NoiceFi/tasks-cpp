#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <iostream>
#include <memory>
#include <stack>
#include <type_traits>
#include <vector>

template <size_t N> struct StackStorage {
  char buffer[N];
  char *free_ptr = buffer;
};

template <typename T, size_t N> struct StackAllocator {
  using value_type = T;

  StackStorage<N> *pool;
  StackAllocator() = default;
  StackAllocator(StackStorage<N> &pool) : pool(&pool) {}
  template <typename U>
  StackAllocator(const StackAllocator<U, N> &other) : pool(other.pool) {}
  ~StackAllocator() {}
  T *allocate(size_t count) {
    size_t space = (pool->buffer + N) - pool->free_ptr;
    pool->free_ptr = reinterpret_cast<char *>(
        std::align(alignof(T), count * sizeof(T),
                   reinterpret_cast<void *&>(pool->free_ptr), space));
    T *copy = reinterpret_cast<T *>(pool->free_ptr);
    pool->free_ptr += sizeof(T) * count;
    return copy;
  }
  void deallocate(T*, size_t) {}

  template <typename U> struct rebind {
    using other = StackAllocator<U, N>;
  };
  bool operator==(const StackAllocator &other) const {
    return pool == other.pool;
  }

  bool operator!=(const StackAllocator &other) const {
    return !(*this == other);
  }
};
template <typename T, typename Allocator = std::allocator<T>> struct List {
  struct BaseNode {
    BaseNode *prev;
    BaseNode *next;
    BaseNode(BaseNode* prev_, BaseNode* next_) : prev(prev_), next(next_) {}
  };
  struct Node : BaseNode {
    T value;
    Node(BaseNode* prev_, BaseNode* next_)
        : BaseNode(prev_, next_), value() {}
    Node(BaseNode* prev_, BaseNode* next_, const T &value_)
        : BaseNode(prev_, next_), value(value_) {}
  };
  mutable BaseNode EndNode;
  size_t sz;
  using NodeAlloc = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
  using Alloc_call = typename std::allocator_traits<NodeAlloc>;
  NodeAlloc Alloc;
  List() : EndNode(nullptr, nullptr), sz(0), Alloc(NodeAlloc()) {}
  List(Allocator other) : EndNode(nullptr, nullptr), sz(0), Alloc(other) {}
  List(size_t count, Allocator other = NodeAlloc()) : List(other) {
    if (count >= 1) {
      EndNode.prev = EndNode.next = CreateNode(&EndNode, &EndNode);
      sz++;
    }
    for (size_t i = 0; i < count - 1; i++) {
      Node *newNode = CreateNode(&EndNode, EndNode.next);
      (*EndNode.next).prev = newNode;
      EndNode.next = newNode;
      sz++;
    }
  }
  List(size_t count, const T &val, Allocator other = NodeAlloc()) : List(other) {
    for (size_t i = 0; i < count; i++) {
      push_back(val);
    }
  }
  List(const List &other)
      : List(std::allocator_traits<
             NodeAlloc>::select_on_container_copy_construction(other.Alloc)) {
    for (auto it = other.begin(); it != other.end(); ++it) {
      push_back(*it);
    }
  }
  ~List() {
    while(sz > 0) {
      pop_front();
    }
  }
  template <typename... Args>
  Node *CreateNode(BaseNode* prev, BaseNode* next, Args &...args) {
    Node *newNode = Alloc_call::allocate(Alloc, 1);
    Alloc_call::construct(Alloc, newNode, prev, next, args...);
    return newNode;
  }
  size_t size() const { return sz; }
  bool empty() const { return (sz == 0); }
  NodeAlloc get_allocator() const { return Alloc; }
  void swap(List &other) {
    std::swap(EndNode, other.EndNode);
    std::swap(sz, other.sz);
    std::swap(Alloc, other.Alloc);
  }
  List &operator=(const List &other) {
    NodeAlloc new_alloc = std::allocator_traits<
        NodeAlloc>::propagate_on_container_copy_assignment::value ? other.get_allocator(): (*this).get_allocator();
    List copy = List(new_alloc);
    try {
      for (auto it = other.begin(); it != other.end(); ++it) {
        copy.push_back((*it));
      }
    } catch (...) {
      throw;
    }
    swap(copy);
    return *this;
  }
  void push_back(const T &value) {
    Node *newNode = nullptr;
    try {
      newNode = Alloc_call::allocate(Alloc, 1);
      Alloc_call::construct(
          Alloc, newNode, (EndNode.prev ? EndNode.prev : &EndNode), &EndNode, value);
    } catch (...) {
      Alloc_call::deallocate(Alloc, newNode, 1);
      throw;
    }
    if (EndNode.prev) {
      EndNode.prev->next = newNode;
    } else {
      EndNode.next = newNode;
    }
    EndNode.prev = newNode;
    sz++;
  }
  void push_front(const T &value) {
    Node *newNode = nullptr;
    try {
      newNode = Alloc_call::allocate(Alloc, 1);
      Alloc_call::construct(
          Alloc, newNode,
          Node{&EndNode, (EndNode.next ? EndNode.next : &EndNode), value});
    } catch (...) {
      Alloc_call::deallocate(Alloc, newNode, 1);
      throw;
    }
    if (EndNode.next) {
      EndNode.next->prev = newNode;
    } else {
      EndNode.prev = newNode;
    }
    EndNode.next = newNode;
    sz++;
  }
  void pop_front() {
    if (sz == 1) {
      Alloc_call::destroy(Alloc, static_cast<Node *>(EndNode.next));
      Alloc_call::deallocate(Alloc, static_cast<Node *>(EndNode.next), 1);
      EndNode.prev = EndNode.next = nullptr;
    } else {
      BaseNode *new_first = EndNode.next->next;
      Alloc_call::destroy(Alloc, static_cast<Node *>(EndNode.next));
      Alloc_call::deallocate(Alloc, static_cast<Node *>(EndNode.next), 1);
      new_first->prev = &EndNode;
      EndNode.next = new_first;
    }
    sz--;
  }
  void pop_back() {
    if (sz == 1) {
      Alloc_call::destroy(Alloc, static_cast<Node *>(EndNode.prev));
      Alloc_call::deallocate(Alloc, static_cast<Node *>(EndNode.prev), 1);
      EndNode.prev = EndNode.next = nullptr;
    } else {
      BaseNode *new_last = EndNode.prev->prev;
      Alloc_call::destroy(Alloc, static_cast<Node *>(EndNode.prev));
      Alloc_call::deallocate(Alloc, static_cast<Node *>(EndNode.prev), 1);
      new_last->next = &EndNode;
      EndNode.prev = new_last;
    }
    sz--;
  }

  template <bool isConst> struct BaseIter {
    using pointer = std::conditional_t<isConst, const Node *, Node *>;
    using reference = std::conditional_t<isConst, const T &, T &>;
    using value_type = std::conditional_t<isConst, const T, T>;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = ptrdiff_t;
    BaseNode *ptr;
    BaseIter(const BaseIter &other) : ptr(other.ptr) {}
    BaseIter(BaseNode *ptr_) : ptr(ptr_) {}
    BaseIter& operator++() {
      ptr = ptr->next;
      return *this;
    }
    BaseIter& operator--() {
      ptr = ptr->prev;
      return *this;
    }
    BaseIter operator++(int) {
      BaseIter copy(*this);
      ++(*this);
      return copy;
    }
    BaseIter operator--(int) {
      BaseIter copy(*this);
      --(*this);
      return copy;
    }
    reference operator*() {
      pointer tmp = static_cast<pointer>(ptr);
      return tmp->value;
    }
    BaseIter &operator=(const BaseIter &other) = default;
    pointer operator->() { return static_cast<Node *>(ptr); }
    bool operator==(const BaseIter &other) const { return ptr == other.ptr; }
    bool operator!=(const BaseIter &other) const { return !((*this) == other); }
    operator BaseIter<true>() const { return {ptr}; }
  };
  using iterator = BaseIter<false>;
  using const_iterator = BaseIter<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  iterator begin() {
    return iterator((EndNode.next ? EndNode.next : &EndNode));
  }
  iterator end() { return iterator(&EndNode); }
  const_iterator cbegin() const {
    return const_iterator((EndNode.next ? EndNode.next : &EndNode));
  }
  const_iterator cend() const { return const_iterator(&EndNode); }
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
  const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }
  const_reverse_iterator rbegin() const { return crbegin(); }
  const_reverse_iterator rend() const { return crend(); }
  void erase(const_iterator it) {
    if (sz == 1) {
      Alloc_call::destroy(Alloc, it.ptr);
      Alloc_call::deallocate(Alloc, static_cast<Node *>(EndNode.next), 1);
      EndNode.next = EndNode.prev = nullptr;
    } else {
      iterator copy(it.ptr);
      iterator left(copy.ptr->prev);
      iterator right(copy.ptr->next);
      left->next = right.ptr;
      right->prev = left.ptr;
      Alloc_call::destroy(Alloc, static_cast<Node*>(copy.ptr));
      Alloc_call::deallocate(Alloc, static_cast<Node *>(copy.ptr), 1);
    }
    sz--;
  }
  void insert(const_iterator it, const T &value) {
    Node *newNode = Alloc_call::allocate(Alloc, 1);
    iterator copy(it.ptr);
    iterator prev(copy.ptr->prev);
    try {
      Alloc_call::construct(Alloc, newNode, Node(prev.ptr, it.ptr, value));
    } catch (...) {
      Alloc_call::destroy(Alloc, newNode);
      Alloc_call::deallocate(Alloc, newNode, 1);
    }
    prev->next = newNode;
    copy->prev = newNode;
    sz++;
  }
};
