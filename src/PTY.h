#pragma once
#include "Object.h"

class PTY : public Object {
public:
  virtual void start_shell(const char *) = 0;

  virtual ~PTY(void) = default;

  static PTY *createPTY(void);
};
