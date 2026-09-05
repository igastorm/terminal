#pragma once
#include "IApplication.hpp"
#include "IObject.hpp"
#include <cstddef>

class IPTY : public IObject {
public:
  virtual void startShell(const char *) = 0;
  virtual void writeInput(const void *, size_t) = 0;

  virtual ~IPTY() = default;
  static IPTY *createPTY(IApplication *);
};
