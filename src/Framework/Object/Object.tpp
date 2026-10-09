#pragma once
#include "Object.hpp"
#include <cstdlib>

template <class Interface> int Object<Interface>::addRef() {
  return ++this->ref_count;
}

template <class Interface> int Object<Interface>::release() {
  return --this->ref_count;
}

template <class Interface> Object<Interface>::Object() { this->addRef(); }

template <class Interface> Object<Interface>::~Object() { std::free(this); }
