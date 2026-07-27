#ifndef __SKY_SKYHEAP_HPP__
#define __SKY_SKYHEAP_HPP__

#include <malloc-2.8.6.h>
#include "Utils/Types.h"
#include "Utils/TSPOS.hpp"
#include "Base/Meta.hpp"

// ----------------------------------------------------------------------------
// [SECTION] HeapTag
// ----------------------------------------------------------------------------

#define HEAP_TAG_DECLARE(_Id) \
  extern HeapTagType *_Id;

#define HEAP_TAG_REGISTER(_Id) \
  HeapTagType *_Id;\
  static HeapTagList g_heapTagList_ ## _Id = {\
    &_Id,\
    # _Id\
  };

// Windows has defined HeapTag as an enum.
struct HeapTagType {
  cstring m_name = "";
  HeapTagType *m_prev = nullptr;
  char m_id[4] = {'H', 'P', 'T', 'G'};
};

class HeapTagList {
private:
  static HeapTagList *&m_List() {
    static HeapTagList *p = nullptr;
    return p;
  };

public:
  HeapTagList(
    HeapTagType **tag,
    cstring name
  )
    : m_tag(tag)
    , m_name(name)
    , m_prev(m_List())
  {
    m_List() = this;
  }

private:
  const char *m_name = nullptr;
  HeapTagType **m_tag = nullptr;
  HeapTagList *m_prev = nullptr;
};

// ----------------------------------------------------------------------------
// [SECTION] Heap
// ----------------------------------------------------------------------------

class Heap;
META_DECLARE_CLASS(Heap)

class Heap: public Object {
public:
  Heap(): Object(MetaClassId(Heap)) { }
  ~Heap() = default;

  inline size_t GetMaxSize() { return m_maxSize; }
  inline bool IsInitialized() { return !!m_mem; }
  size_t GetUsedBytes(bool refresh);

  void Initialize(
    void *memory,
    size_t maxSize,
    const char *name,
    bool isClearMemory,
    bool isClearAllowed);

  void Clear();

  void *Allocate(
    size_t size,
    HeapTagType *tag,
    size_t align);

  void Free(
    void *p);

private:
  inline void m_BeginLock() { if (m_lock) m_lock->BeginLock(); }
  inline void m_EndLock() { if (m_lock) m_lock->EndLock(); }

  void m_TryStlAllocate(
    void **pBlock,
    size_t size,
    HeapTagType *tag,
    size_t align);

  // The memory space reserved for the mspace.
  void *m_base = nullptr;
  // Max capacity of the heap.
  size_t m_maxSize = 0;
  // The total sum of used memory.
  size_t m_usedBytes = 0;
  // This field records the size of the mspace memory used.
  size_t m_mspaceUsedSize = 0;
  // If the mspace is exhausted but the heap maximum capacity has not been reached,
  // use STL for allocation. This field records the size of the STL memory used.
  size_t m_stlUsedSize = 0;
  // Identifier of the heap.
  char m_name[30] = {0};
  char unk_2 = 0;
  bool m_clearMemory = false;
  // True if mspace is exhausted.
  bool m_isMemExhausted = false;
  char m_isStatic = false;
  // Heap::Clear is allowed if true.
  char m_isClearAllowed = false;
  // Handle of mspace.
  mspace m_mem = nullptr;
  // Handle of mutex.
  Lock *m_lock = nullptr;
};

#endif
