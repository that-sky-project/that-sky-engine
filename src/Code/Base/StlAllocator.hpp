#ifndef __STLALLOCATOR_HPP__
#define __STLALLOCATOR_HPP__

#pragma warning(disable: 28251)

void *operator new(size_t _Size);
void *operator new[](size_t _Size);
void operator delete(void *_Block) noexcept;
void operator delete[](void *_Block) noexcept;

#endif
