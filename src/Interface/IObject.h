#pragma once

class IObject {
public:
  virtual int release(void) = 0;

  virtual int addRef(void) = 0;

  virtual ~IObject(void) = default;
};
