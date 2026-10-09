#pragma once

template <class Interface> class Object : public Interface {
private:
  int ref_count = 0;

protected:
  virtual int addRef() override;
  virtual int release() override;
  Object();
  virtual ~Object();
};

#include "Object.tpp"
