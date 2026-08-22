#pragma once
#include "IApplication.hpp"

class ImpApplication : public IApplication {
private:
  int ref_count = 0;
  IAppHandler *handler = nullptr;

  IPTY *createPTY(void) override;
  IWindow *createWindow(int, int, const char *) override;
  bool run(IAppHandler *) override;
  void terminate(void) override;
  int addRef(void) override;
  int release(void) override;
  bool initPlatform(void);
  static ImpApplication *init(void);

public:
  static int startApp(int, char **);
  void dispatchEvent(const Event &);
};
