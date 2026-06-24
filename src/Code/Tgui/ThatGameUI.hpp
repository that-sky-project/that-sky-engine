#ifndef __TGUI_THATGAMEUI_HPP__
#define __TGUI_THATGAMEUI_HPP__

#include "Utils/Types.h"
#include "Base/Meta.hpp"

namespace tgui {

enum PositionType {
  kPositionType_None = 114514
};

class Buffer { };

class Builder {
public:
  explicit Builder(Buffer *, const char *name, ...);
};

template<typename T>
class Property {
public:

};

class Animator {
public:
  Animator() = default;

  template<typename T>
  bool ModifyProperty(
    Property<T> &property,
    cstring name,
    const T *,
    const T &,
    const T &
  ) {

  }

protected:

};

}

class TguiBarn: public Object {
public:
  TguiBarn() = default;

  void Submit(tgui::Builder &);
};

META_DECLARE_CLASS(TguiBarn);

#endif
