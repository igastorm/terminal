#pragma once
#include "IApplication.hpp"

class CommonApplication : public IApplication {
private:
  int ref_count = 0;

protected:
  IAppHandler *handler = nullptr;

  virtual ~CommonApplication() = default;
  friend CommonApplication *createPlatformApplication();

public:
  int addRefBase();
  int release() override;

  static int startApp(int, char **);
};

extern CommonApplication *createPlatformApplication();
