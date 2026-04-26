#ifndef __STLALLOCATOR_HPP__
#define __STLALLOCATOR_HPP__

#include <new>

#pragma warning(disable: 28251)

void *operator new(size_t _Size);
void *operator new[](size_t _Size);
void operator delete(void *_Block) noexcept;
void operator delete[](void *_Block) noexcept;

void *operator new(size_t _Size, const std::nothrow_t &_Tag) noexcept;
void *operator new[](size_t _Size, const std::nothrow_t &_Tag) noexcept;
void operator delete(void *_Block, const std::nothrow_t &_Tag) noexcept;
void operator delete[](void *_Block, const std::nothrow_t &_Tag) noexcept;

#endif
