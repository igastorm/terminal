#pragma once
#include "IAppHandler.hpp"
#include "IObject.hpp"

// 利用側で release とかするとまずいので IObject は private
class IApplication : private IObject {
public:
  virtual bool run(const char *, IAppHandler *) = 0;
  virtual void terminate() = 0;

  virtual void postEvent() = 0;

  virtual ~IApplication() = default;
};

extern int appMain(int, char **, IApplication *);
