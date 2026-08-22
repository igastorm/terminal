#pragma once

class IObject {
public:
  virtual int release() = 0;

  virtual int addRef() = 0;

  virtual ~IObject() = default;
};
