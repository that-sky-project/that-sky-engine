#include <new>
#include "Utils/Assert.hpp"
#include "Utils/StlAllocator.hpp"

// - StlAllocator Base Functions

void *StlAllocator::Allocate(
  size_t size,
  size_t align,
  size_t minAlign
) {
  if (!size) return nullptr;
  if (align) minAlign = align;
  if (minAlign < 0x10) minAlign = 0x10;
  return _aligned_malloc(size, minAlign);
}

void StlAllocator::Free(
  void *block
) {
  if (block) _aligned_free(block);
}

size_t StlAllocator::GetSize(
  void *block
) {
  if (!block)
    return 0;

  size_t size = _aligned_msize(block, 0x10, 0);
  SkyAssert(size != 0);

  return size;
}

// - Operator Overrides

// Basic allocator override.
void *operator new(
  size_t _Size
) {
  if (!_Size)
    _Size = 1;

  void *raw = StlAllocator::Allocate(_Size, 0x10, 0x10);
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
  return StlAllocator::Free(_Block);
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
  return StlAllocator::Allocate(_Size, 0x10, 0x10);
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


