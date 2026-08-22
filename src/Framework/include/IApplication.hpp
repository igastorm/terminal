#pragma once
#include "IAppHandler.hpp"
#include "IObject.hpp"
#include "IWindow.hpp"

// 利用側で release とかするとまずいので IObject は private
class IApplication : private IObject {
public:
  virtual IWindow *createWindow(int, int, const char *) = 0;
  virtual bool run(IAppHandler *) = 0;
  virtual void terminate(void) = 0;

  virtual ~IApplication(void) = default;
};

extern int appMain(int, char **, IApplication *);
