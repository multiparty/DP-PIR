#ifndef DPPIR_TYPES_CONTAINERS_H_
#define DPPIR_TYPES_CONTAINERS_H_

#include <cassert>
#include <cstring>
#include <iterator>
#include <memory>

#include "DPPIR/types/types.h"

namespace DPPIR {

// Maps server_id to desired value.
// The keys are contigious except that server_id_ is a gap.
template <typename T>
class ServersMap {
 public:
  // Fill with default constructor.
  ServersMap(server_id_t server_id, server_id_t server_count)
      : server_id_(server_id), data_(std::make_unique<T[]>(server_count - 1)) {}

  // Fill with copy constructor.
  ServersMap(server_id_t server_id, server_id_t server_count, T fill)
      : ServersMap(server_id, server_count) {
    for (server_id_t i = 0; i < server_id; i++) {
      this->data_[i] = fill;
    }
    for (server_id_t i = server_id + 1; i < server_count; i++) {
      this->data_[i - 1] = fill;
    }
  }

  // Look up.
  inline T& operator[](server_id_t server_id) {
    assert(server_id != this->server_id_);
    if (server_id > this->server_id_) {
      server_id--;
    }
    return this->data_[server_id];
  }
  inline const T& operator[](server_id_t server_id) const {
    assert(server_id != this->server_id_);
    if (server_id > this->server_id_) {
      server_id--;
    }
    return this->data_[server_id];
  }

  // Unprotected access to underlying ptr.
  inline T* Ptr() { return this->data_.get(); }

 private:
  server_id_t server_id_;
  std::unique_ptr<T[]> data_;
};

// Buffer with compile-time capacity SZ along with a read/write index.
template <std::size_t SZ>
using PhysicalBuffer = std::array<char, SZ>;

// Logical buffer: does not manage the physical buffer. Just abstracts
// it into a buffer for a given type.
template <typename T>
class LogicalBuffer {
 public:
  // Default constructor: for use in containers.
  LogicalBuffer() : buf_(nullptr), capacity_(0), end_(0), leftover_(0) {}

  template <std::size_t SZ>
  explicit LogicalBuffer(PhysicalBuffer<SZ>* buf)
      : buf_(reinterpret_cast<T*>(buf->data())),
        capacity_(SZ / sizeof(T)),
        end_(0),
        leftover_(0) {}

  // Iterator API.
  using iterator = T*;
  inline iterator begin() { return this->buf_; }
  inline iterator end() { return this->buf_ + this->end_; }

  // Capacity and size.
  inline bool Full() const { return this->end_ == this->capacity_; }
  inline size_t Size() const { return this->end_; }
  inline void Clear() {
    if (this->end_ > 0) {
      T* incomplete_ptr = this->buf_ + this->end_;
      memcpy(reinterpret_cast<char*>(this->buf_),
             reinterpret_cast<char*>(incomplete_ptr), this->leftover_);
    }
    this->end_ = 0;
  }
  inline void Update(size_t bytes_read) {
    bytes_read += this->leftover_;
    this->leftover_ = bytes_read % sizeof(T);
    this->end_ = bytes_read / sizeof(T);
  }

  // To plain C buffer.
  inline char* ToBuffer() {
    return reinterpret_cast<char*>(this->buf_) + this->leftover_;
  }
  inline size_t BufferSize() { return this->end_ * sizeof(T); }
  inline size_t BufferCapacity() {
    return this->capacity_ * sizeof(T) - this->leftover_;
  }
  inline size_t UnitSize() { return sizeof(T); }
  inline size_t Leftover() { return this->leftover_; }

  // Element access.
  inline T& operator[](index_t idx) { return this->buf_[idx]; }
  inline void PushBack(const T& v) { this->buf_[this->end_++] = v; }

 private:
  T* buf_;
  index_t capacity_;
  index_t end_;  // Marks the index of the first unused/incomplete T in buf_.
  size_t leftover_;
};

// Batch with run-time capacity. Basically a thin vector.
template <typename T>
class Batch {
 public:
  Batch() : ptr_(nullptr), capacity_(0), left_(0), right_(0) {}

  // Creation/deletion.
  void Initialize(index_t capacity) {
    this->ptr_ = std::make_unique<T[]>(capacity);
    this->capacity_ = capacity;
    this->left_ = 0;
    this->right_ = 0;
  }
  void Free() {
    this->ptr_ = nullptr;
    this->capacity_ = 0;
    this->left_ = 0;
    this->right_ = 0;
  }

  // Iterator API.
  using iterator = T*;
  inline iterator begin() { return this->ptr_.get(); }
  inline iterator end() { return this->ptr_.get() + this->capacity_; }

  // Element access.
  inline T& operator[](index_t idx) { return this->ptr_[idx]; }
  inline void PushBack(const T& v) { this->ptr_[this->right_++] = v; }

  // Capacity and size.
  inline bool Full() const { return this->right_ == this->capacity_; }
  inline index_t Capacity() const { return this->capacity_; }

 private:
  std::unique_ptr<T[]> ptr_;
  index_t capacity_;
  index_t left_;
  index_t right_;
};

// Special iterator for iterating over dynamic-sized ciphers in a buffer.
class CipherIterator {
 public:
  // Constructor.
  CipherIterator(char* ptr, size_t ciphersize)
      : ptr_(ptr), ciphersize_(ciphersize) {}

  // Increment.
  CipherIterator& operator++() {
    this->ptr_ += this->ciphersize_;
    return *this;
  }
  CipherIterator operator++(int) {
    CipherIterator retval = *this;
    ++(*this);
    return retval;
  }
  // Equality
  bool operator==(const CipherIterator& o) const {
    return this->ptr_ == o.ptr_;
  }
  bool operator!=(const CipherIterator& o) const {
    return this->ptr_ != o.ptr_;
  }
  // Access.
  char* operator*() { return this->ptr_; }

  // iterator traits
  using difference_type = index_t;
  using value_type = char*;
  using pointer = char**;
  using reference = char*&;
  using iterator_category = std::forward_iterator_tag;

 private:
  char* ptr_;
  size_t ciphersize_;
};

// Like LogicalBuffer but specialized to ciphers, which have dynamiac size.
class CipherLogicalBuffer {
 public:
  // Default constructor: for use in containers.
  CipherLogicalBuffer()
      : buf_(nullptr), ciphersize_(0), capacity_(0), end_(0), leftover_(0) {}

  template <std::size_t SZ>
  CipherLogicalBuffer(PhysicalBuffer<SZ>* buf, size_t ciphersize)
      : buf_(buf->data()),
        ciphersize_(ciphersize),
        capacity_(SZ - (SZ % ciphersize)),
        end_(0),
        leftover_(0) {}

  // Iterator API.
  inline CipherIterator begin() {
    return CipherIterator(this->buf_, this->ciphersize_);
  }
  inline CipherIterator end() {
    return CipherIterator(this->buf_ + this->end_, this->ciphersize_);
  }

  // Capacity and size (in units of cipher, not char).
  inline bool Full() const { return this->end_ == this->capacity_; }
  inline index_t Size() const { return this->end_ / this->ciphersize_; }
  inline void Clear() {
    if (this->end_ > 0) {
      memcpy(this->buf_, this->buf_ + this->end_, this->leftover_);
    }
    this->end_ = 0;
  }
  inline void Update(size_t bytes_read) {
    bytes_read += this->leftover_;
    this->leftover_ = bytes_read % this->ciphersize_;
    this->end_ = bytes_read - this->leftover_;
  }

  // To plain C buffer.
  inline char* ToBuffer() { return this->buf_ + this->leftover_; }
  inline size_t BufferSize() { return this->end_; }
  inline size_t BufferCapacity() { return this->capacity_ - this->leftover_; }
  inline size_t UnitSize() { return this->ciphersize_; }
  inline size_t Leftover() { return this->leftover_; }

  // Element access.
  inline char* operator[](index_t idx) {
    return this->buf_ + (idx * this->ciphersize_);
  }
  inline void PushBack(const char* v) {
    memcpy(this->buf_ + this->end_, v, this->ciphersize_);
    this->end_ += this->ciphersize_;
  }

 private:
  char* buf_;
  size_t ciphersize_;
  size_t capacity_;
  size_t end_;
  size_t leftover_;
};

// Like Batch but specialized for ciphers.
// A cipher batch has two parts.
// The first part consists of ciphers that have been processed by the party.
// The second consists of ciphers yet to be processed by the party.
// Processed ciphers are shorter (one layer has been taken out of them).
// We store all ciphers in a contigious flat array for cache efficiency.
// Initially, the first part consists of only the ciphers generated by the
// party for its own noise queries, while the second part is filled by
// ciphers read by the party from the previous party.
// The boundary between the two parts moves as ciphers in the second part
// get decrypted and processed, transforming them into ciphers of the first
// kind, until the entire batch consists of only ciphers of the first kind,
// followed by a bunch of unused memory corresponding to the difference
// in size between input and output ciphers.
class CipherBatch {
 public:
  explicit CipherBatch(size_t short_cipher_size, size_t long_cipher_size)
      : ptr_(nullptr),
        last_short_ptr_(nullptr),
        first_long_ptr_(nullptr),
        last_long_ptr_(nullptr),
        end_(nullptr),
        short_cipher_size_(short_cipher_size),
        long_cipher_size_(long_cipher_size) {}

  // Creation/deletion.
  void Initialize(index_t short_cipher_count, index_t long_cipher_count) {
    size_t short_bytes = short_cipher_count * this->short_cipher_size_;
    size_t long_bytes = long_cipher_count * this->long_cipher_size_;
    this->ptr_ = std::make_unique<char[]>(short_bytes + long_bytes);
    this->last_short_ptr_ = this->ptr_.get();
    this->first_long_ptr_ = this->ptr_.get() + short_bytes;
    this->last_long_ptr_ = this->ptr_.get() + short_bytes;
    this->end_ = this->ptr_.get() + short_bytes + long_bytes;
  }
  void Free() {
    this->ptr_ = nullptr;
    this->last_short_ptr_ = nullptr;
    this->first_long_ptr_ = nullptr;
    this->last_long_ptr_ = nullptr;
    this->end_ = nullptr;
  }

  // Full
  inline bool FullLong() const { return this->last_long_ptr_ == this->end_; }
  inline bool HasLong() const {
    return this->first_long_ptr_ < this->last_long_ptr_;
  }

  // Element access.
  inline void PushShort(const char* v) {
    memcpy(this->last_short_ptr_, v, this->short_cipher_size_);
    this->last_short_ptr_ += this->short_cipher_size_;
  }
  inline void PushLong(const char* v) {
    memcpy(this->last_long_ptr_, v, this->long_cipher_size_);
    this->last_long_ptr_ += this->long_cipher_size_;
  }
  inline char* PopLong() {
    char* cipher = this->first_long_ptr_;
    this->first_long_ptr_ += this->long_cipher_size_;
    return cipher;
  }
  inline char* GetShort(index_t idx) {
    return this->ptr_.get() + (idx * this->short_cipher_size_);
  }
  inline void SetShort(index_t idx, const char* v) {
    char* target = this->ptr_.get() + (idx * this->short_cipher_size_);
    memcpy(target, v, this->short_cipher_size_);
  }

  // Iterator API.
  inline CipherIterator begin() {
    return CipherIterator(this->ptr_.get(), this->short_cipher_size_);
  }
  inline CipherIterator end() {
    if (this->last_short_ptr_ == this->ptr_.get()) {
      return CipherIterator(this->first_long_ptr_, this->short_cipher_size_);
    } else {
      return CipherIterator(this->last_short_ptr_, this->short_cipher_size_);
    }
  }

 private:
  std::unique_ptr<char[]> ptr_;
  char* last_short_ptr_;
  char* first_long_ptr_;
  char* last_long_ptr_;
  char* end_;
  size_t short_cipher_size_;
  size_t long_cipher_size_;
};

}  // namespace DPPIR

#endif  // DPPIR_TYPES_CONTAINERS_H_
