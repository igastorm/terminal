#pragma once
#include "IObject.h"

class IPTY : public IObject {
public:
  virtual void start_shell(const char *) = 0;

  virtual ~IPTY(void) = default;
};
