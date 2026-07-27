#ifndef __STLALLOCATOR_HPP__
#define __STLALLOCATOR_HPP__

#include <new>
#include <stddef.h>

#if defined(_MSC_VER)
#pragma warning(disable: 28251)
#endif

namespace StlAllocator {

void *Allocate(size_t size, size_t align, size_t minAlign);
void Free(void *block);
size_t GetSize(void *block);

}

void *operator new(::size_t _Size);
void *operator new[](::size_t _Size);
void operator delete(void *_Block) noexcept;
void operator delete[](void *_Block) noexcept;

void *operator new(::size_t _Size, const std::nothrow_t &_Tag) noexcept;
void *operator new[](::size_t _Size, const std::nothrow_t &_Tag) noexcept;
void operator delete(void *_Block, const std::nothrow_t &_Tag) noexcept;
void operator delete[](void *_Block, const std::nothrow_t &_Tag) noexcept;

#endif
