#pragma once
#include "IObject.hpp"
#include <concepts>

template <class Interface>
  requires std::derived_from<Interface, IObject>
class Object : public Interface {
private:
  int ref_count = 0;

protected:
  virtual int addRef() override;
  virtual int release() override;
  Object();
  virtual ~Object();
};

#include "Object.tpp"
