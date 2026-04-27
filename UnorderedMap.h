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

template <typename Key, typename Value, typename Hash = std::hash<Key>,
  typename Equal = std::equal_to<Key>,
  typename Allocator = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap;

template <typename Key, typename Value>
struct HashNode {
  std::pair<const Key, Value> point;
  size_t hash;

  explicit HashNode(const HashNode& other)
    : point(other.point), hash(other.hash) {}

  HashNode(const std::pair<const Key, Value>& p, size_t h)
    : point(p), hash(h) {}

  HashNode(std::pair<const Key, Value>&& p, size_t h)
    : point(std::move(p)), hash(h) {}

  HashNode(std::pair<Key, Value>&& p, size_t h)
    : point(std::move(p)), hash(h) {}
};

template <typename ListKey, typename ListValue, typename ListHash,
  typename ListAlloc = std::allocator<std::pair<const ListKey, ListValue>>>
class List {

private:

  using T = HashNode<ListKey, ListValue>;

  struct BaseNode {
    BaseNode* prev;
    BaseNode* next;
    BaseNode(BaseNode* prev_, BaseNode* next_) : prev(prev_), next(next_) {}
  };

  struct Node : BaseNode {
    T value;

    Node(BaseNode* prev_, BaseNode* next_) : BaseNode(prev_, next_), value() {}

    template <typename... Args>
    Node(BaseNode* prev_, BaseNode* next_, Args&&... args)
      : BaseNode(prev_, next_), value(std::forward<Args>(args)...) {}
  };

  ListHash hasher;
  BaseNode EndNode;
  size_t sz;

  using NodeAlloc =
    typename std::allocator_traits<ListAlloc>::template rebind_alloc<Node>;
  using Alloc_call = typename std::allocator_traits<NodeAlloc>;
  ListAlloc Value_alloc;
  using Value_Alloc_call = typename std::allocator_traits<ListAlloc>;
  NodeAlloc Alloc;

public:

  List() : EndNode(nullptr, nullptr), sz(0), Alloc(NodeAlloc()) {}

  explicit List(ListAlloc other)
    : EndNode(nullptr, nullptr), sz(0), Alloc(other) {}

  List(size_t count, ListAlloc other = NodeAlloc()) : List(other) {
    if (count >= 1) {
      EndNode.prev = EndNode.next = CreateNode(&EndNode, &EndNode);
      sz++;
    }
    for (size_t i = 0; i < count - 1; i++) {
      Node* newNode = CreateNode(&EndNode, EndNode.next);
      (*EndNode.next).prev = newNode;
      EndNode.next = newNode;
      sz++;
    }
  }

  List(size_t count, const T& val, ListAlloc other = NodeAlloc())
    : List(other) {
    for (size_t i = 0; i < count; i++) {
      push_back(val);
    }
  }

  List(const List& other)
    : List(std::allocator_traits<
      NodeAlloc>::select_on_container_copy_construction(other.Alloc)) {
    for (auto it = other.begin(); it != other.end(); ++it) {
      push_back(*it);
    }
  }

  List(List&& other) noexcept
    : EndNode(other.EndNode), sz(other.sz), Alloc(std::move(other.Alloc)) {
    other.EndNode.prev = other.EndNode.next = nullptr;
    other.sz = 0;
  }

  ~List() {
    while (sz > 0) {
      pop_front();
    }
  }

  size_t size() const { return sz; }

  bool empty() const { return (sz == 0); }

  NodeAlloc get_allocator() const { return Alloc; }

  void swap(List& other) {
    std::swap(EndNode, other.EndNode);
    if (EndNode.next != nullptr) {
      EndNode.next->prev = EndNode.prev->next = &EndNode;
    }
    std::swap(sz, other.sz);
    if (std::allocator_traits<NodeAlloc>::propagate_on_container_swap::value) {
      std::swap(Alloc, other.Alloc);
    }
  }

  List& operator=(const List& other) {
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

  List& operator=(List&& other) noexcept {
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

  template <typename... Args>
  void emplace_back(Args&&... args) {
    Node* newNode = nullptr;
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

  template <typename... Args>
  void emplace_front(Args&&... args) {
    Node* newNode = nullptr;
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

  void push_back(const T& value) { emplace_back(value.point); }

  void push_back(T&& value) { emplace_back(std::move(value.point)); }

  void push_front(const T& value) { empace_front(value.point); }

  void push_front(T&& value) { emplace_front(std::move(value.point)); }

  void pop_front() {
    if (sz == 1) {
      Alloc_call::destroy(Alloc, static_cast<Node*>(EndNode.next));
      Alloc_call::deallocate(Alloc, static_cast<Node*>(EndNode.next), 1);
      EndNode.prev = EndNode.next = nullptr;
    } else {
      BaseNode* new_first = EndNode.next->next;
      Alloc_call::destroy(Alloc, static_cast<Node*>(EndNode.next));
      Alloc_call::deallocate(Alloc, static_cast<Node*>(EndNode.next), 1);
      new_first->prev = &EndNode;
      EndNode.next = new_first;
    }
    sz--;
  }

  void pop_back() {
    if (sz == 1) {
      Alloc_call::destroy(Alloc, static_cast<Node*>(EndNode.prev));
      Alloc_call::deallocate(Alloc, static_cast<Node*>(EndNode.prev), 1);
      EndNode.prev = EndNode.next = nullptr;
    } else {
      BaseNode* new_last = EndNode.prev->prev;
      Alloc_call::destroy(Alloc, static_cast<Node*>(EndNode.prev));
      Alloc_call::deallocate(Alloc, static_cast<Node*>(EndNode.prev), 1);
      new_last->next = &EndNode;
      EndNode.prev = new_last;
    }
    sz--;
  }

  template <bool isConst>
  struct BaseIter {
    using pointer = std::conditional_t<isConst, const Node*, Node*>;
    using value_type = std::conditional_t<isConst, const T, T>;
    using reference = value_type&;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = ptrdiff_t;
    using ptr_type = std::conditional_t<isConst, const BaseNode*, BaseNode*>;

    ptr_type ptr;

    BaseIter(const BaseIter& other) : ptr(other.ptr) {}

    BaseIter(ptr_type ptr_) : ptr(ptr_) {}

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

    BaseIter& operator=(const BaseIter& other) = default;

    pointer operator->() { return static_cast<pointer>(ptr); }

    bool operator==(const BaseIter& other) const { return ptr == other.ptr; }

    bool operator!=(const BaseIter& other) const { return !((*this) == other); }

    operator BaseIter<true>() const { return { ptr }; }
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
      Alloc_call::deallocate(Alloc, static_cast<Node*>(EndNode.next), 1);
      EndNode.next = EndNode.prev = nullptr;
    } else {
      iterator copy(it.ptr);
      iterator left(copy.ptr->prev);
      iterator right(copy.ptr->next);
      left->next = right.ptr;
      right->prev = left.ptr;
      Alloc_call::destroy(Alloc, static_cast<Node*>(copy.ptr));
      Alloc_call::deallocate(Alloc, static_cast<Node*>(copy.ptr), 1);
    }
    sz--;
  }

  void insert(const_iterator it, T&& value) {
    emplace_insert(it, std::move(value));
  }

  void insert(const_iterator it, const T& value) { emplace_insert(it, value); }

private:

  template <typename... Args>
  Node* CreateNode(BaseNode* prev, BaseNode* next, Args&&... args) {
    Node* newNode = Alloc_call::allocate(Alloc, 1);
    Value_Alloc_call::construct(Value_alloc, &(newNode->value.point),
      std::forward<Args>(args)...);
    newNode->value.hash = hasher(newNode->value.point.first);
    newNode->prev = prev;
    newNode->next = next;
    return newNode;
  }

  void DestroyAndDeallocate(BaseNode* ptr) {
    Value_Alloc_call::destroy(Value_alloc, &(static_cast<Node*>(ptr)->value));
    Alloc_call::deallocate(Alloc, static_cast<Node*>(ptr), 1);
  }

  template <typename... Args>
  void emplace_insert(const_iterator it, Args&&... args) {
    iterator copy(it.ptr);
    iterator prev(copy.ptr->prev);
    Node* newNode = nullptr;
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

  void cut_out(BaseNode* ptr) {
    ptr->prev->next = ptr->next;
    ptr->next->prev = ptr->prev;
    ptr->next = ptr->prev = nullptr;
    sz--;
  }

  void insert_node(BaseNode* left, BaseNode* right, BaseNode* mid) {
    left->next = mid;
    mid->prev = left;
    mid->next = right;
    right->prev = mid;
    sz++;
  }

  template <typename Key, typename Value, typename Hash,
    typename Equal,
    typename Allocator>
  friend class UnorderedMap;

};

template <typename Key, typename Value, typename Hash,
  typename Equal,
  typename Allocator>
class UnorderedMap {
private:
  [[no_unique_address]] Hash hasher;
  [[no_unique_address]] Equal equal;
  size_t hashing(const Key& x) { return hasher(x); }
  [[no_unique_address]] Allocator alloc;
  List<Key, Value, Hash, Allocator> list;
  using NodeAlloc =
    typename std::allocator_traits<Allocator>::template rebind_alloc<
    typename List<Key, Value, Hash, Allocator>::Node*>;

  std::vector<typename List<Key, Value, Hash, Allocator>::Node*, NodeAlloc> buck;
  double max_factor = 0.8;

  using HashMapList = List<Key, Value, Hash, Allocator>;
  using NodeType = std::pair<const Key, Value>;
  using ListIterator = typename List<Key, Value, Hash, Allocator>::iterator;
  using ListNode = typename List<Key, Value, Hash, Allocator>::Node;
  using ListBaseNode = typename List<Key, Value, Hash, Allocator>::BaseNode;

  std::pair<const Key, Value>& get_pair(ListBaseNode* ptr) {
    return static_cast<ListNode*>(ptr)->value;
  }

  const Key& get_key(ListBaseNode* ptr) {
    return static_cast<ListNode*>(ptr)->value.point.first;
  }

  Value& get_value(ListBaseNode* ptr) {
    return static_cast<ListNode*>(ptr)->value.point.second;
  }

  size_t get_hash(ListBaseNode* ptr) {
    return static_cast<ListNode*>(ptr)->value.hash;
  }

  ListNode* get_node_ptr(ListBaseNode* ptr) {
    return static_cast<ListNode*>(ptr);
  }

  void check_load_factor() {
    if (load_factor() > max_factor) {
      std::random_device rd;
      std::mt19937 rnd(rd());
      size_t x = rnd() % 5 + 1;
      size_t y = rnd() % 5 + 1;
      rehash(buck.size() * x + y);
    }
  }

  double load_factor() { return size() / buck.size(); }

  Value& add_new_bucket(size_t pos) {
    buck[pos] = begin().get_list_node();
    return begin()->second;
  }

public:

  size_t size() const { return list.size(); }

  bool empty() const { return (list.size() == 0); }

  UnorderedMap() : hasher(), equal(), alloc(), list(alloc), buck(1, alloc) {}

  explicit UnorderedMap(Allocator other)
    : hasher(), equal(), alloc(other), list(alloc), buck(1, alloc) {}

  UnorderedMap(const UnorderedMap& other)
    : hasher(other.hasher),
    equal(other.equal),
    alloc(
      std::allocator_traits<Allocator>::select_on_container_copy_construction(
        other.alloc)),
    list(other.list),
    buck() {
    buck.resize(other.buck.size(), nullptr);
    for (auto it = begin(), End = end(); it != End; ++it) {
      size_t pos = it.get_hash() % buck.size();
      if (buck[pos] == nullptr) {
        buck[pos] = static_cast<ListNode*>(it.ptr);
      }
    }
  }

  UnorderedMap(UnorderedMap&& other) noexcept
    : hasher(std::move(other.hasher)),
    equal(std::move(other.equal)),
    list(std::move(other.list)),
    buck(std::move(other.buck)) {}

  UnorderedMap& operator=(UnorderedMap&& other) noexcept {
    hasher = std::move(other.hasher);
    equal = std::move(other.equal);
    list = std::move(other.list);
    buck = std::move(other.buck);
    return *this;
  }

  UnorderedMap& operator=(const UnorderedMap& other) {
    hasher = other.hasher;
    equal = other.equal;
    list = other.list;
    buck.clear();
    buck.resize(other.buck.size(), nullptr);
    for (auto it = begin(), End = end(); it != End; ++it) {
      size_t pos = it.get_hash() % buck.size();
      if (buck[pos] == nullptr) {
        buck[pos] = static_cast<ListNode*>(it.ptr);
      }
    }
    return *this;
  }

  template <bool is_const>
  struct base_iterator : HashMapList::template BaseIter<is_const> {

    using value_type = std::conditional_t<is_const, const NodeType, NodeType>;
    using pointer = value_type*;
    using reference = value_type&;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using IterType =
      std::conditional_t<is_const,
      typename HashMapList::template BaseIter<true>,
      typename HashMapList::template BaseIter<false>>;

    pointer operator->() { return &((*IterType::operator->()).value.point); }

    reference operator*() { return IterType::operator*().point; }

    base_iterator(const IterType& other) : IterType(other) {}

    base_iterator(const base_iterator& other) = default;

    operator base_iterator<true>() const { return { IterType::ptr }; }

    base_iterator& operator=(const base_iterator& other) = default;

    size_t& get_hash() { return IterType::operator*().hash; }

    ListNode* get_list_node() { return static_cast<ListNode*>(IterType::ptr); }

  };

  using iterator = base_iterator<false>;
  using const_iterator = base_iterator<true>;
  iterator begin() { return { list.begin() }; }
  iterator end() { return { list.end() }; }
  const_iterator cbegin() const { return const_iterator{ list.begin() }; }
  const_iterator cend() const { return const_iterator{ list.end() }; }
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }

  double& max_load_factor() { return max_factor; }

  void reserve(size_t count) { return rehash(std::ceil(count / max_factor)); }

  iterator find(const Key& key) {
    size_t hash = hasher(key);
    size_t pos = hash % buck.size();
    if (buck[pos]) {
      for (auto it = iterator(static_cast<ListBaseNode*>(buck[pos])),
        End = end();
        it != End && it.get_hash() == hash; ++it) {
        if (equal(it->first, key)) {
          return it;
        }
      }
    }
    return end();
  }

  Value& at(const Key& key) {
    auto it = find(key);
    if (it == end()) {
      throw std::out_of_range("Oops");
    }
    return it->second;
  }

  Value& operator[](const Key& key) {
    auto it = find(key);
    if (it != end()) {
      return it->second;
    }
    return emplace(key, Value()).first->second;
  }

  Value& operator[](Key&& key) {
    auto it = find(key);
    if (it != end()) {
      return it->second;
    }
    return emplace(std::move(key), Value()).first->second;
  }

  void rehash(size_t new_bucket_count) {
    buck.clear();
    buck.resize(new_bucket_count, nullptr);
    std::vector<ListBaseNode*> order;
    for (auto it = list.begin(), end = list.end(); it != end; ++it) {
      order.push_back(it.ptr);
    }
    for (auto& node : order) {
      size_t pos = get_hash(node) % buck.size();
      if (buck[pos] == nullptr) {
        buck[pos] = get_node_ptr(node);
      } else {
        list.cut_out(node);
        list.insert_node(buck[pos], buck[pos]->next, node);
      }
    }
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    auto newNode = list.CreateNode(nullptr, nullptr, std::forward<Args>(args)...);
    size_t hash = newNode->value.hash;
    size_t pos = hash % buck.size();
    ListNode* ptr = buck[pos];
    if (!ptr) {
      list.insert_node(&list.EndNode, 
        (list.EndNode.next ? list.EndNode.next : &list.EndNode), newNode);
      buck[pos] = begin().get_list_node();
      return { begin(), true };
    }
    iterator it(ptr);
    for (iterator End = end(); it != End; ++it) {
      bool hash_is_same = (it.get_hash() % buck.size() == hash % buck.size());
      bool is_equal = equal(it->first, newNode->value.point.first);
      if (is_equal) {
        list.DestroyAndDeallocate(newNode);
        return { it, false };
      }
      if (!hash_is_same) {
        list.insert_node(it.ptr->prev, it.ptr, newNode);
        check_load_factor();
        return { --it, true };
      }
    }
    list.insert_node((list.EndNode.prev ? list.EndNode.prev : &list.EndNode),
      &list.EndNode, newNode);
    check_load_factor();
    return { --end(), true };
  }

  std::pair<iterator, bool> insert(NodeType& x) { return emplace(x); }

  std::pair<iterator, bool> insert(std::pair<Key, Value>&& x) {
    return emplace(std::move(x));
  }

  template <typename InputIter>
  void insert(InputIter l, InputIter r) {
    for (; l != r; ++l) {
      emplace(*l);
    }
    check_load_factor();
  }

  iterator erase(iterator it) {
    size_t pos = it.get_hash() % buck.size();
    if (buck[pos] == it.ptr) {
      auto copy = it;
      if (++copy == end()) {
        buck[pos] = nullptr;
      } else {
        size_t next_pos = copy.get_hash() % buck.size();
        buck[pos] =
          (pos != next_pos) ? nullptr : copy.get_list_node();
      }
    }
    auto ans = it;
    ans++;
    list.erase(it);
    return ans;
  }

  iterator erase(iterator first, iterator last) {
    while (first != last) {
      first = erase(first);
    }
    return last;
  }
};
