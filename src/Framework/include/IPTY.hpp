#pragma once
#include "IObject.hpp"

class IPTY : public IObject {
public:
  virtual void start_shell(const char *) = 0;

  virtual ~IPTY(void) = default;
};
