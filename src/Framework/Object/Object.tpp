#pragma once
#include "IObject.hpp"
#include "Object.hpp"
#include <concepts>
#include <cstdlib>

template <class Interface>
  requires std::derived_from<Interface, IObject>
int Object<Interface>::addRef() {
  return ++this->ref_count;
}

template <class Interface>
  requires std::derived_from<Interface, IObject>
int Object<Interface>::release() {
  return --this->ref_count;
}

template <class Interface>
  requires std::derived_from<Interface, IObject>
Object<Interface>::Object() {
  this->addRef();
}

template <class Interface>
  requires std::derived_from<Interface, IObject>
Object<Interface>::~Object() {
  std::free(this);
}
