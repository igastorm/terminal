#pragma once
#include "IApplication.hpp"

class CommonApplication : public IApplication {
private:
  int ref_count = 0;

protected:
  IAppHandler *handler = nullptr;

  CommonApplication();
  friend CommonApplication *createPlatformApplication();

public:
  int addRef() override;
  int release() override;

  virtual ~CommonApplication() = default;

  static int startApp(int, char **);
};

extern CommonApplication *createPlatformApplication();
