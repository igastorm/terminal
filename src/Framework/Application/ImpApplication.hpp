#pragma once
#include "CommonApplication.hpp"

template <class PlatformData> class ImpApplication : public CommonApplication {
private:
  int ref_count = 0;
  IAppHandler *handler = nullptr;
  PlatformData data;

  IWindow *createWindow(int, int, const char *) override;
  bool run(IAppHandler *) override;
  void terminate(void) override;

public:
  void dispatchEvent(const Event &);
  bool initPlatform(void);
};
