#ifndef __UTILS_AUTOLISTER_HPP__
#define __UTILS_AUTOLISTER_HPP__

template<typename T>
class AutoLister {
private:
  struct Vars {

  };

  static Vars ms_vars;

public:

private:
  AutoLister<T> *m_next = nullptr;
  AutoLister<T> *m_prev = nullptr;
};

#endif