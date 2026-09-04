#pragma once
#include "CommonApplication.hpp"

template <class PlatformData> class ImpApplication : public CommonApplication {
private:
  PlatformData data;

  IWindow *createWindow(int, int, const char *) override;
  IGraphicsDevice* createGraphicsDevice() override;
  bool run(IAppHandler *) override;
  void terminate() override;

public:
  void dispatchEvent(const Event &);
  bool initPlatform();
  int addRef() override;
};
