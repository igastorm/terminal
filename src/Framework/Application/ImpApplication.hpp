#pragma once
#include "CommonApplication.hpp"

template <class PlatformData> class ImpApplicationTemplate : public CommonApplication {
protected:
  PlatformData data;

  IWindow *createWindow(int, int, const char *) override;
  IGraphicsDevice* createGraphicsDevice() override;
  bool run(IAppHandler *) override;
  void postEvent() override;

public:
  void terminate() override;
  
  void dispatchEvent(const Event &);
};
