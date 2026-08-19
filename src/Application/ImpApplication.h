#pragma once
#include "IApplication.h"

class ImpApplication : public IApplication {
private:
  int ref_count = 0;

  IPTY *createPTY(void) override;
  IWindow *createWindow(int, int, const char *) override;
  int addRef(void) override;
  int release(void) override;

public:
  static ImpApplication *Init(void);
};
