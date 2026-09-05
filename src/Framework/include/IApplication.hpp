#pragma once
#include "IAppHandler.hpp"
#include "IGraphicsDevice.hpp"
#include "IObject.hpp"
#include "IWindow.hpp"

// 利用側で release とかするとまずいので IObject は private
class IApplication : private IObject {
public:
  virtual IWindow *createWindow(int, int, const char *) = 0;
  virtual bool run(IAppHandler *) = 0;
  virtual void terminate() = 0;

  virtual IGraphicsDevice *createGraphicsDevice() = 0;

  virtual void postEvent() = 0;

  virtual ~IApplication() = default;
};

extern int appMain(int, char **, IApplication *);
