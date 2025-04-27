#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <iostream>
#include <memory>
#include <random>
#include <stack>
#include <string>
#include <type_traits>
#include <vector>
std::mt19937 rnd(1);
template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>,
          typename ALLOC = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
public:
  Hash hasher;
  Equal eq;
  size_t hashing(const Key &x) { return hasher(x); }
  struct HashNode {
    std::pair<const Key, Value> point;
    size_t hash;
    // HashNode(HashNode &&other)
    //     : point(std::move(other.point)), hash(other.hash) {}
    HashNode(const HashNode &other) : point(other.point), hash(other.hash) {}
    HashNode(const std::pair<const Key, Value> &p, size_t h)
        : point(p), hash(h) {}
    HashNode(std::pair<const Key, Value> &&p, size_t h)
        : point(std::move(p)), hash(h) {}
    HashNode(std::pair<Key, Value> &&p, size_t h)
        : point(std::move(p)), hash(h) {}
  };
  using T = HashNode;
  /*using PointerNodeAlloc = typename std::allocator_traits<
      Alloc>::template rebind_alloc<List<HashNode>::template Node *>;*/
  template <typename Allocator = std::allocator<std::pair<const Key, Value>>>
  struct List {
    struct BaseNode {
      BaseNode *prev;
      BaseNode *next;
      BaseNode(BaseNode *prev_, BaseNode *next_) : prev(prev_), next(next_) {}
    };
    struct Node : BaseNode {
      T value;
      Node(BaseNode *prev_, BaseNode *next_)
          : BaseNode(prev_, next_), value() {}
      template <typename... Args>
      Node(BaseNode *prev_, BaseNode *next_, Args &&...args)
          : BaseNode(prev_, next_), value(std::forward<Args>(args)...) {}
    };
    Hash hasher;
    mutable BaseNode EndNode;
    size_t sz;
    using NodeAlloc =
        typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    using Alloc_call = typename std::allocator_traits<NodeAlloc>;
    Allocator Value_alloc;
    using Value_Alloc_call = typename std::allocator_traits<Allocator>;
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
    List(size_t count, const T &val, Allocator other = NodeAlloc())
        : List(other) {
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
    List(List &&other)
        : EndNode(other.EndNode), sz(other.sz), Alloc(std::move(other.Alloc)) {
      other.EndNode.prev = other.EndNode.next = nullptr;
      other.sz = 0;
    }
    ~List() {
      while (sz > 0) {
        pop_front();
      }
    }
    template <typename... Args>
    Node *CreateNode(BaseNode *prev, BaseNode *next, Args &...args) {
      Node *newNode = Alloc_call::allocate(Alloc, 1);
      Alloc_call::construct(Alloc, newNode, prev, next, args...);
      return newNode;
    }
    size_t size() const { return sz; }
    bool empty() const { return (sz == 0); }
    NodeAlloc get_allocator() const { return Alloc; }
    void swap(List &other) {
      std::swap(EndNode, other.EndNode);
      if (EndNode.next != nullptr) {
        EndNode.next->prev = EndNode.prev->next = &EndNode;
      }
      std::swap(sz, other.sz);
      if (std::allocator_traits<
              NodeAlloc>::propagate_on_container_swap::value) {
        std::swap(Alloc, other.Alloc);
      }
    }
    List &operator=(const List &other) {
      NodeAlloc new_alloc =
          std::allocator_traits<
              NodeAlloc>::propagate_on_container_copy_assignment::value
              ? other.get_allocator()
              : (*this).get_allocator();
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
    List &operator=(List &&other) {
      NodeAlloc new_alloc =
          std::allocator_traits<
              NodeAlloc>::propagate_on_container_move_assignment::value
              ? other.get_allocator()
              : (*this).get_allocator();
      List copy(new_alloc);
      copy.EndNode = other.EndNode;
      other.EndNode.next = other.EndNode.prev = nullptr;
      if (copy.EndNode.next != nullptr) {
        copy.EndNode.next->prev = copy.EndNode.prev->next = &copy.EndNode;
      }
      copy.sz = other.sz;
      other.sz = 0;
      swap(copy);
      return *this;
    }
    template <typename... Args> void emplace_back(Args &&...args) {
      Node *newNode = nullptr;
      try {
        newNode = Alloc_call::allocate(Alloc, 1);
        Value_Alloc_call::construct(Value_alloc, &(newNode->value.point),
                                    std::forward<Args>(args)...);
        newNode->value.hash = hasher(newNode->value.point.first);
        newNode->prev = (EndNode.prev ? EndNode.prev : &EndNode);
        newNode->next = &EndNode;
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
    template <typename... Args> void emplace_front(Args &&...args) {
      Node *newNode = nullptr;
      try {
        newNode = Alloc_call::allocate(Alloc, 1);
        Value_Alloc_call::construct(Value_alloc, &(newNode->value.point),
                                    std::forward<Args>(args)...);
        newNode->value.hash = hasher(newNode->value.point.first);
        newNode->prev = &EndNode;
        newNode->next = (EndNode.next ? EndNode.next : &EndNode);
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
    void push_back(const T &value) { emplace_back(value.point); }
    void push_back(T &&value) { emplace_back(std::move(value.point)); }
    void push_front(const T &value) { empace_front(value.point); }
    void push_front(T &&value) { emplace_front(std::move(value.point)); }
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
      BaseIter &operator++() {
        ptr = ptr->next;
        return *this;
      }
      BaseIter &operator--() {
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
      bool operator!=(const BaseIter &other) const {
        return !((*this) == other);
      }
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
    const_reverse_iterator crbegin() const {
      return const_reverse_iterator(cend());
    }
    const_reverse_iterator crend() const {
      return const_reverse_iterator(cbegin());
    }
    const_reverse_iterator rbegin() const { return crbegin(); }
    const_reverse_iterator rend() const { return crend(); }
    void erase(iterator it) {
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
        Alloc_call::destroy(Alloc, static_cast<Node *>(copy.ptr));
        Alloc_call::deallocate(Alloc, static_cast<Node *>(copy.ptr), 1);
      }
      sz--;
    }
    template <typename... Args>
    void emplace_insert(const_iterator it, Args &&...args) {
      iterator copy(it.ptr);
      iterator prev(copy.ptr->prev);
      Node *newNode = nullptr;
      try {
        newNode = Alloc_call::allocate(Alloc, 1);
        Value_Alloc_call::construct(Value_alloc, &(newNode->value.point),
                                    std::forward<Args>(args)...);
        newNode->value.hash = hasher(newNode->value.point.first);
        newNode->prev = prev.ptr;
        newNode->next = it.ptr;
      } catch (...) {
        Alloc_call::deallocate(Alloc, newNode, 1);
        throw;
      }
      prev->next = newNode;
      copy->prev = newNode;
      sz++;
    }
    void insert(const_iterator it, T &&value) {
      emplace_insert(it, std::move(value));
    }
    void insert(const_iterator it, const T &value) {
      emplace_insert(it, value);
    }

    void cut_out(BaseNode *ptr) {
      ptr->prev->next = ptr->next;
      ptr->next->prev = ptr->prev;
      ptr->next = ptr->prev = nullptr;
      sz--;
    }
    void insert_node(BaseNode *left, BaseNode *right, BaseNode *mid) {
      left->next = mid;
      mid->prev = left;
      mid->next = right;
      right->prev = mid;
      sz++;
    }
  };

  ALLOC alloc;
  List<ALLOC> ls;
  using NodeAlloc = typename std::allocator_traits<
      ALLOC>::template rebind_alloc<typename List<ALLOC>::Node *>;
  std::vector<typename List<ALLOC>::Node *, NodeAlloc> buck;
  using HashMapList = List<ALLOC>;
  using NodeType = std::pair<const Key, Value>;
  using ListIterator = typename List<ALLOC>::iterator;
  using ListNode = typename List<ALLOC>::Node;
  using ListBaseNode = typename List<ALLOC>::BaseNode;
  std::pair<const Key, Value> &get_pair(ListBaseNode *ptr) {
    return static_cast<ListNode *>(ptr)->value;
  }
  const Key &get_key(ListBaseNode *ptr) {
    return static_cast<ListNode *>(ptr)->value.point.first;
  }
  Value &get_value(ListBaseNode *ptr) {
    return static_cast<ListNode *>(ptr)->value.point.second;
  }
  size_t get_hash(ListBaseNode *ptr) {
    return static_cast<ListNode *>(ptr)->value.hash;
  }
  ListNode *get_node_ptr(ListBaseNode *ptr) {
    return static_cast<ListNode *>(ptr);
  }
  size_t size() const { return ls.size(); }
  bool empty() const { return (ls.size() == 0); }
  UnorderedMap() : hasher(), eq(), alloc(), ls(alloc), buck(1, alloc) {}
  UnorderedMap(ALLOC other)
      : hasher(), eq(), alloc(other), ls(alloc), buck(1, alloc) {}
  UnorderedMap(const UnorderedMap &other)
      : UnorderedMap(
            std::allocator_traits<ALLOC>::select_on_container_copy_construction(
                other.alloc)) {
    hasher = other.hasher;
    eq = other.eq;
    ls = other.ls;
    buck.clear();
    buck.resize(other.buck.size(), nullptr);
    for (auto it = begin(), End = end(); it != End; ++it) {
      size_t pos = it.get_hash() % buck.size();
      if (buck[pos] == nullptr) {
        buck[pos] = static_cast<ListNode *>(it.iter.ptr);
      }
    }
  }
  UnorderedMap(UnorderedMap &&other)
      : hasher(other.hasher), eq(other.eq), ls(std::move(other.ls)),
        buck(std::move(other.buck)) {}

  UnorderedMap &operator=(UnorderedMap &&other) {
    hasher = other.hasher;
    eq = other.eq;
    ls = std::move(other.ls);
    buck = std::move(other.buck);
    return *this;
  }
  UnorderedMap &operator=(const UnorderedMap &other) {
    hasher = other.hasher;
    eq = other.eq;
    ls = other.ls;
    buck.clear();
    buck.resize(other.buck.size(), nullptr);
    for (auto it = begin(), End = end(); it != End; ++it) {
      size_t pos = it.get_hash() % buck.size();
      if (buck[pos] == nullptr) {
        buck[pos] = static_cast<ListNode *>(it.iter.ptr);
      }
    }
    return *this;
  }
  template <bool isConst> struct base_iterator {
    using pointer = std::conditional_t<isConst, const NodeType *, NodeType *>;
    using reference = std::conditional_t<isConst, const NodeType &, NodeType &>;
    using value_type = std::conditional_t<isConst, const NodeType, NodeType>;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using IterType = std::conditional_t<
        isConst,
        typename HashMapList::template BaseIter<true>,
        typename HashMapList::template BaseIter<false>
        >;
    IterType iter;
    base_iterator(const IterType &other) : iter(other) {}
    base_iterator(const base_iterator &other) : iter(other.iter) {}
    base_iterator(ListBaseNode *ptr) : iter(ptr) {}
    base_iterator &operator++() {
      iter++;
      return *this;
    }
    base_iterator &operator--() {
      iter--;
      return *this;
    }
    base_iterator operator++(int) {
      base_iterator copy(*this);
      ++(*this);
      return copy;
    }
    base_iterator operator--(int) {
      base_iterator copy(*this);
      --(*this);
      return copy;
    }
    size_t &get_hash() { return iter->value.hash; }
    IterType get_iter() { return iter; }
    reference operator*() { return iter->value.point; }
    base_iterator &operator=(const base_iterator &other) = default;
    pointer operator->() { return &iter->value.point; }
    bool operator==(const base_iterator &other) const {
      return iter == other.iter;
    }
    bool operator!=(const base_iterator &other) const {
      return !((*this) == other);
    }
    operator base_iterator<true>() const { return {iter}; }
  };
  using iterator = base_iterator<false>;
  using const_iterator = base_iterator<true>;
  iterator begin() { return {ls.begin()}; }
  iterator end() { return {ls.end()}; }
  const_iterator cbegin() const { return const_iterator{ls.begin()}; }
  const_iterator cend() const { return const_iterator{ls.end()}; }
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }

  double max_load_factor() { return 0.8; }
  double load_factor() {
    double tmp = size();
    return tmp / buck.size();
  }
  void reserve(size_t count) {
    return rehash(std::ceil(count / max_load_factor()));
  }
  void check_load_factor() {
    if (load_factor() > max_load_factor()) {
      size_t x = rnd() % 5 + 1;
      size_t y = rnd() % 5 + 1;
      rehash(buck.size() * x + y);
    }
  }
  iterator find(const Key &key) {
    size_t hash = hasher(key);
    size_t pos = hash % buck.size();
    if (buck[pos]) {
      // for (auto it = begin(); it != end(); it++) {
      //   if (it.get_hash() == hash) {
      //     std::cout << 1 << "\n";
      //   }
      // }
      for (auto it = iterator(static_cast<ListBaseNode *>(buck[pos])),
                End = end();
           it != End && it.get_hash() == hash; ++it) {
        if (eq(it->first, key)) {
          return it;
        }
      }
    }
    return end();
  }
  Value &add_new_bucket(size_t pos) {
    buck[pos] = static_cast<ListNode *>(begin().get_iter().ptr);
    return begin()->second;
  }

  Value &at(const Key &key) {
    auto it = find(key);
    if (it == end()) {
      throw std::out_of_range("Oops");
    }
    return it->second;
  }

  Value &operator[](const Key &key) {
    auto it = find(key);
    if (it != end()) {
      return it->second;
    }
    return emplace(key, Value()).first->second;
  }

  Value &operator[](Key &&key) {
    auto it = find(key);
    if (it != end()) {
      return it->second;
    }
    return emplace(std::move(key), Value()).first->second;
  }
  void rehash(size_t new_bucket_count) {
    buck.clear();
    buck.resize(new_bucket_count, nullptr);
    std::vector<ListBaseNode *> order;
    for (auto it = ls.begin(), end = ls.end(); it != end; ++it) {
      order.push_back(it.ptr);
    }
    for (auto &t : order) {
      size_t pos = get_hash(t) % buck.size();
      if (buck[pos] == nullptr) {
        buck[pos] = get_node_ptr(t);
      } else {
        ls.cut_out(t);
        ls.insert_node(buck[pos], buck[pos]->next, t);
      }
    }
  }
  template <typename... Args>
  std::pair<iterator, bool> emplace(Args &&...args) {
    ls.emplace_front(std::forward<Args>(args)...);
    size_t hash = begin().get_hash();
    size_t pos = hash % buck.size();
    ListNode *ptr = buck[pos];
    if (!ptr) {
      buck[pos] = static_cast<ListNode *>(begin().iter.ptr);
      return {begin(), true};
    }
    iterator it(ptr);
    auto Begin = begin();
    for (iterator End = end(); it != End; ++it) {
      bool f = (it.get_hash() % buck.size() == hash % buck.size());
      bool equal = eq(it->first, Begin->first);
      if (equal) {
        ls.pop_front();
        return {it, false};
      }
      if (!f) {
        if (it == begin()) {
          return {begin(), true};
        } else {
          ls.cut_out(Begin.iter.ptr);
          ls.insert_node(it.iter.ptr->prev, it.iter.ptr, Begin.iter.ptr);
          check_load_factor();
          return {--it, true};
        }
      }
    }
    ls.cut_out(Begin.iter.ptr);
    ls.insert_node(ls.EndNode.prev, &ls.EndNode, Begin.iter.ptr);
    check_load_factor();
    return {--end(), true};
  }
  std::pair<iterator, bool> insert(NodeType &x) {
    return emplace(x);
  }
  std::pair<iterator, bool> insert(std::pair<Key, Value> &&x) {
    return emplace(std::move(x));
  }
  template <typename InputIter> void insert(InputIter l, InputIter r) {
    for (; l != r; ++l) {
      emplace(*l);
    }
    check_load_factor();
  }
  iterator erase(iterator it) {
    size_t pos = it.get_hash() % buck.size();
    if (buck[pos] == it.iter.ptr) {
      auto copy = it;
      size_t next_pos = (++copy).get_hash() % buck.size();
      buck[pos] =
          (pos != next_pos) ? nullptr : static_cast<ListNode *>(copy.iter.ptr);
    }
    auto ans = it;
    ans++;
    ls.erase(it.iter);
    return ans;
  }
  iterator erase(iterator first, iterator last) {
    while (first != last) {
      first = erase(first);
    }
    return last;
  }
};

