#pragma once
#include "IApplication.hpp"

class Application : public IApplication {
private:
  int ref_count = 0;

protected:
  IAppHandler *handler = nullptr;

  Application();
  friend Application *createPlatformApplication();

public:
  int addRef() override;
  int release() override;

  virtual ~Application() = default;

  static int startApp(int, char **);
};

extern Application *createPlatformApplication();
