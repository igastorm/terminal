#pragma once

class Object {
public:
  virtual int release(void) = 0;

  virtual int addRef(void) = 0;
};
