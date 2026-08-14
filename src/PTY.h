#pragma once

class PTY {
public:
  virtual void start_shell(char *) = 0;

  virtual int release(void) = 0;

  virtual int addRef(void) = 0;

  virtual ~PTY(void) = default;

  static PTY *createPTY(void);
};
