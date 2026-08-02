#include <malloc-2.8.6.h>
#include "Utils/Assert.hpp"
#include "Utils/StlAllocator.hpp"
#include "Memory/Heap.hpp"

META_REGISTER_CLASS(Heap)

size_t Heap::GetUsedBytes(
  bool refresh
) {
  if (refresh) {
    m_BeginLock();

    struct mallinfo info = mspace_mallinfo(m_mem);
    if (m_usedBytes <= info.uordblks)
      m_usedBytes = info.uordblks;

    m_EndLock();

    return info.uordblks;
  }

  if (m_usedBytes <= m_mspaceUsedSize)
    m_usedBytes = m_mspaceUsedSize;

  return m_stlUsedSize + m_mspaceUsedSize;
}

void Heap::Initialize(
  void *memory,
  size_t maxSize,
  const char *name,
  bool isClearMemory,
  bool isClearAllowed
) {
  SkyAssert(!IsInitialized());
  SkyAssertMsg(
    ((intptr_t)memory & 0xF) == 0,
    "Heap %s memory should be 16 byte aligned. %p is not.",
    name,
    memory);
  SkyAssertMsg(
    (maxSize % 16) == 0,
    "Heap %s sizes should be 16 byte aligned/padded. %zu is not.",
    name, 
    maxSize);

  m_clearMemory = isClearMemory;
  m_isClearAllowed = isClearAllowed;
  m_base = memory;
  m_maxSize = maxSize;
  m_usedBytes = 0;

  if (isClearMemory)
    memset(m_base, 0, m_maxSize);

  strncpy(m_name, name, 30);

  m_mem = create_mspace_with_base(m_base, m_maxSize, 0);
  SkyAssertMsg(m_mem, "Failed creating mspace. Try increasing heap %s size", name);

  mspace_set_footprint_limit(m_mem, m_maxSize);

  m_isStatic = !strcmp(name, "Static");

  // Ignored the shared lock and lister read/write below.
}

void Heap::Clear() {
  SkyAssert(m_isClearAllowed);

  m_BeginLock();

  if (m_mem)
    destroy_mspace(m_mem);

  if (m_clearMemory)
    memset(m_base, 0, m_maxSize);

  m_mem = create_mspace_with_base(m_base, m_maxSize, 0i64);
  mspace_set_footprint_limit(m_mem, m_maxSize);
  
  m_mspaceUsedSize = m_stlUsedSize = 0;

  m_EndLock();
}

void *Heap::Allocate(
  size_t size,
  HeapTagType *tag,
  size_t align
) {
  if (!size)
    return nullptr;

  if (m_maxSize <= size)
    SkyAssertMsg(false, "Heap %s invalid alloc size larger than heap %zu/%zu", m_name, size, m_maxSize);

  m_BeginLock();

  void *ptr = mspace_memalign(m_mem, align, size + 8);
  if (ptr) {
    *(HeapTagType **)((char *)ptr + mspace_usable_size(ptr) - 8) = tag;
    m_mspaceUsedSize += mspace_usable_size(ptr);
  }

  m_EndLock();

  m_TryStlAllocate(&ptr, size, tag, align);

  return ptr;
}

void Heap::Free(
  void *block
) {
  if (!block)
    return;

  if (
    (uintptr_t)m_base > (uintptr_t)block
    || (uintptr_t)m_base + m_maxSize <= (uintptr_t)block
  ) {
    // Outside of the base memory area, free the memory block with StlAllocator.
    m_stlUsedSize -= StlAllocator::GetSize(block);
    StlAllocator::Free(block);
  } else {
    m_BeginLock();
    m_mspaceUsedSize -= mspace_usable_size(block);
    mspace_free(m_mem, block);
    m_EndLock();
  }
}

void Heap::m_TryStlAllocate(
  void **pBlock,
  size_t size,
  HeapTagType *tag,
  size_t align
) {
  if (*pBlock)
    return;

  if (!m_isMemExhausted) {
    m_isMemExhausted = true;
    // Ignored logger.
  }

  void *block = StlAllocator::Allocate(size, align, 1);
  if (!block) {
    SkyAssertMsg(
      false,
      "Heap %s gAllocAligned failed size %zu for heap tag \"%s\"",
      m_name,
      size,
      tag->m_name);
  }

  *pBlock = block;
  m_stlUsedSize += StlAllocator::GetSize(block);
}
