#include <new>
#include "StlAllocator.hpp"

// Basic allocator override.
void *operator new(
  size_t _Size
) {
  if (!_Size)
    _Size = 1;

  void *raw = _aligned_malloc(_Size, 0x10);
  if (!raw)
    throw std::bad_alloc();

  return raw;
}

void *operator new[](size_t _Size) {
  return operator new(_Size);
}

void operator delete(
  void *_Block
) noexcept {
  return _aligned_free(_Block);
}

void operator delete[](
  void *_Block
) noexcept {
  return operator delete(_Block);
}

// Nothrow allocator override.
void *operator new(
  std::size_t _Size,
  const std::nothrow_t &
) noexcept {
  if (!_Size)
    _Size = 1;

  return _aligned_malloc(_Size, 0x10);
}

void *operator new[](
  size_t _Size,
  const std::nothrow_t &_Tag
) noexcept {
  return operator new(_Size, _Tag);
}

void operator delete(
  void *_Block,
  const std::nothrow_t &
) noexcept {
  return operator delete(_Block);
}

void operator delete[](
  void *_Block,
  const std::nothrow_t &
) noexcept {
  return operator delete(_Block);
}


