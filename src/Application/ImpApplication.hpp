#pragma once
#include "IApplication.hpp"

class ImpApplication : public IApplication {
private:
  int ref_count = 0;

  IPTY *createPTY(void) override;
  IWindow *createWindow(int, int, const char *) override;
  int addRef(void) override;
  int release(void) override;
  static bool initPlatform(void);
   static ImpApplication *init(void);

public:
  static int startApp(int, char**);
};
