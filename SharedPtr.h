#include <iostream>

template <typename T>
class SharedPtr;
template <typename T>
class WeakPtr;

struct ControlBlock {

  size_t count_shared = 1;
  size_t count_weak = 0;
  bool life_object = true;

  size_t use_count() const { return count_shared; }

  void decrement_shared() {
    count_shared--;
    if (count_shared == 0) {
      if (count_weak == 0) {
        delete_block();
      } else {
        delete_object();
      }
    }
  }

  void decrement_weak() {
    count_weak--;
    if (count_shared == 0 && count_weak == 0) {
      delete_block();
    }
  }

  void increase_shared() { count_shared++; }

  void increase_weak() { count_weak++; }

  virtual void delete_block() { delete this; }

  virtual void delete_object() {}

  virtual ~ControlBlock() {}

};

template <typename U>
struct ValueControlBlock : ControlBlock {

  U object;

  template <typename... Args>
  ValueControlBlock(Args&&... args)
    : ControlBlock(), object(std::forward<Args>(args)...) {}

  void delete_object() override {
    ControlBlock::life_object = false;
    object.~U();
  }
  void delete_block() override { delete this; }

  ~ValueControlBlock() {}

};

template <typename U>
struct PtrControlBlock : ControlBlock {
  U* object;

  PtrControlBlock(U* ptr) : ControlBlock(), object(ptr) {}

  void delete_object() override {
    ControlBlock::life_object = false;
    delete object;
  }

  void delete_block() override {
    if (ControlBlock::life_object)
      delete object;
    delete this;
  }

  ~PtrControlBlock() override {}

};

template <typename U, typename Deleter>
struct BlockWithDeleter : PtrControlBlock<U> {
  Deleter del;

  BlockWithDeleter(U* ptr, Deleter del) : PtrControlBlock<U>(ptr), del(del) {}

  void delete_object() override {
    del(PtrControlBlock<U>::object);
    ControlBlock::life_object = false;
  }

  void delete_block() override {
    if (ControlBlock::life_object) {
      del(PtrControlBlock<U>::object);
    }
    delete this;
  }

};

template <typename U, typename Allocator>
struct BlockWithAllocator : ValueControlBlock<U> {
  Allocator alloc;

  template <typename... Args>
  BlockWithAllocator(Allocator a, Args&&... args)
    : ValueControlBlock<U>(std::forward<Args>(args)...), alloc(a) {}

  void delete_object() override {
    using ObjectAllocator =
      typename std::allocator_traits<Allocator>::template rebind_alloc<U>;
    ObjectAllocator delAlloc(alloc);
    std::allocator_traits<ObjectAllocator>::destroy(delAlloc, &(this->object));
    ControlBlock::life_object = false;
  }

  void delete_block() override {
    using BlockAllocator = typename std::allocator_traits<
      Allocator>::template rebind_alloc<BlockWithAllocator>;
    BlockAllocator delAlloc(alloc);
    if (ControlBlock::life_object) {
      std::allocator_traits<BlockAllocator>::destroy(delAlloc, this);
    }
    delAlloc.deallocate(this, 1);
  }

};

template <typename U, typename Deleter, typename Allocator>
struct BlockWithAllocatorAndDeleter : PtrControlBlock<U> {
  Deleter del;
  Allocator alloc;

  BlockWithAllocatorAndDeleter(U* ptr, Deleter d, Allocator alloc)
    : PtrControlBlock<U>(ptr), del(d), alloc(alloc) {}

  void delete_object() override {
    del(PtrControlBlock<U>::object);
  }

  void delete_block() override {
    using BlockAllocator = typename std::allocator_traits<
      Allocator>::template rebind_alloc<BlockWithAllocatorAndDeleter>;
    BlockAllocator delAlloc(alloc);
    if (ControlBlock::life_object) {
      del(PtrControlBlock<U>::object);
    }
    delAlloc.deallocate(this, 1);
  }

};

template <typename T>
class SharedPtr {
  
  private:

  T* ptr;
  ControlBlock* block;

  public:

  T& operator*() const {
    return *ptr;
  }

  T* operator->() const {
    return ptr;
  }

  size_t use_count() const { return block->use_count(); }

  T* get() const {
    return block ? ptr : nullptr;
  }

  SharedPtr() : ptr(nullptr), block(nullptr) {}

  SharedPtr(T* ptr) : ptr(ptr), block(new PtrControlBlock<T>(ptr)) {}

  SharedPtr(std::pair<T*, ControlBlock*> fields) : ptr(fields.first), block(fields.second) {}

  SharedPtr(const SharedPtr<T>& r, T* ptr) : ptr(ptr), block(r.block) {}

  template <typename U, typename Deleter>
  SharedPtr(U* ptr, Deleter del)
    : ptr(static_cast<T*>(ptr)), block(new BlockWithDeleter<U, Deleter>(ptr, del)) {}

  template <typename U>
  SharedPtr(const SharedPtr<U>& other)
    : ptr(other.ptr), block(other.block) {
    block->increase_shared();
  }

  SharedPtr(const SharedPtr& other) : ptr(other.ptr), block(other.block) {
    if (block)
      block->increase_shared();
  }

  template <typename U>
  SharedPtr(SharedPtr<U>&& other) : ptr(other.ptr), block(other.block) {
    other.ptr = nullptr;
    other.block = nullptr;
  }

  SharedPtr(SharedPtr&& other) : ptr(other.ptr), block(other.block) { 
    other.ptr = nullptr;
    other.block = nullptr;
  }

  template<typename U, typename Allocator, typename Deleter>
  SharedPtr(U* pointer, Deleter del, Allocator alloc) {
    using BlockAllocator = typename std::allocator_traits<
      Allocator>::template rebind_alloc<BlockWithAllocatorAndDeleter<U, Deleter, Allocator>>;
    BlockAllocator block_alloc(alloc);
    auto p = block_alloc.allocate(1);
    new (p) BlockWithAllocatorAndDeleter<U, Deleter, Allocator>(pointer, del, alloc);
    ptr = pointer;
    block = p;
  }

  ~SharedPtr() {
    if (block)
      block->decrement_shared();
  }

  template <typename U>
  SharedPtr& operator=(const SharedPtr<U>& other) {
    if (block)
      block->decrement_shared();
    block = other.block;
    ptr = other.ptr;
    block->increase_shared();
    return *this;
  }

  SharedPtr& operator=(const SharedPtr& other) {
    if (block)
      block->decrement_shared();
    block = other.block;
    ptr = other.ptr;
    block->increase_shared();
    return *this;
  }

  template <typename U>
  SharedPtr& operator=(SharedPtr<U>&& other) {
    if (block)
      block->decrement_shared();
    block = other.block;
    ptr = other.ptr;
    other.block = nullptr;
    return *this;
  }

  SharedPtr& operator=(SharedPtr&& other) {
    if (block)
      block->decrement_shared();
    block = other.block;
    ptr = other.ptr;
    other.block = nullptr;
    return *this;
  }

  template <typename U>
  void reset(U* pointer) {
    if (block)
      block->decrement_shared();
    block = new PtrControlBlock<U>(pointer);
    ptr = pointer;
  }

  void reset() {
    if (block)
      block->decrement_shared();
    ptr = nullptr;
    block = nullptr;
  }

  template <typename U>
  void swap(SharedPtr<U>& other) {
    std::swap(block, other.block);
    std::swap(ptr, other.ptr);
  }

  template <typename... Args> friend SharedPtr<T> makeShared(Args&&...);

  template <typename Allocator, typename... Args>
  friend SharedPtr<T> allocateShared(Allocator, Args&&...);

  template <typename U> friend class WeakPtr;

  template <typename U> friend class SharedPtr;

private:

  SharedPtr(const WeakPtr<T>& other) : SharedPtr<T>(std::make_pair(other.ptr, other.block)) {}
};

template <typename T, typename Allocator, typename... Args>
SharedPtr<T> allocateShared(Allocator Alloc, Args&&... args) {
  using OurBlock = BlockWithAllocator<T, Allocator>;
  using BlockAllocator = typename std::allocator_traits<
    Allocator>::template rebind_alloc<OurBlock>;
  BlockAllocator alloc(Alloc);
  auto ptr = alloc.allocate(1);
  alloc.construct(ptr, Alloc, std::forward<Args>(args)...);
  return SharedPtr<T>(std::make_pair(&ptr->object, ptr));
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args &&...args) {
  auto block_ptr = new
    BlockWithAllocator<T, std::allocator<T>>(
      std::allocator<T>(), std::forward<Args>(args)...);
  return SharedPtr<T>(std::make_pair(&block_ptr->object, block_ptr));
   
}

template <typename T>
class WeakPtr {
private:

  T* ptr;
  ControlBlock* block;

public:

  WeakPtr() : ptr(nullptr), block(nullptr) {}

  template <typename U>
  WeakPtr(const SharedPtr<U>& other) : ptr(other.ptr), block(other.block) {
    if (block)
      block->increase_weak();
  }

  template <typename U>
  WeakPtr(const WeakPtr<U>& other) : ptr(other.ptr), block(other.block) {
    if (block)
      block->increase_weak();
  }

  template <typename U>
  WeakPtr(WeakPtr<U>&& other) : ptr(other.ptr), block(other.block) {
    other.block = nullptr;
    other.ptr = nullptr;
  }

  WeakPtr(const WeakPtr& other) : ptr(other.ptr), block(other.block) {
    if (block)
      block->increase_weak();
  }

  WeakPtr(WeakPtr&& other) : ptr(other.ptr), block(other.block)  { 
    other.ptr = nullptr;
    other.block = nullptr;
  }

  template <typename U>
  WeakPtr& operator=(const WeakPtr<U>& other) {
    if (block)
      block->decrement_weak();
    ptr = other.ptr;
    block = other.block;
    block->increase_weak();
    return *this;
  }

  ~WeakPtr() {
    if (block)
      block->decrement_weak();
  }

  template <typename U>
  WeakPtr& operator=(WeakPtr<U>&& other) {
    if (block)
      block->decrement_weak();
    ptr = other.ptr;
    block = other.block;
    other.block = nullptr;
    return *this;
  }

  template <typename U>
  WeakPtr& operator=(const SharedPtr<U>& other) {
    if (block)
      block->decrement_weak();
    ptr = other.ptr;
    block = other.block;
    block->increase_weak();
    return *this;
  }

  SharedPtr<T> lock() const {
    if (block)
      block->increase_shared();
    return SharedPtr<T>(*this);
  }

  bool expired() const {
    if (!block)
      return false;
    return block->use_count() == 0;
  }

  size_t use_count() {
    if (!block)
      return 0;
    return block->use_count();
  }

  template<typename U> friend class SharedPtr;

  template<typename U> friend class WeakPtr;

};

template <typename T>
struct EnableSharedFromThis {
  WeakPtr<T> weak_ptr;
  SharedPtr<T> shared_from_this() { return weak_ptr.lock(); }
};
