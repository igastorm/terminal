#pragma once
#include "IAppHandler.hpp"

class IApplication{
public:
  virtual bool run(const char *, IAppHandler *) = 0;
  virtual void terminate() = 0;

  virtual void postEvent() = 0;

  ~IApplication() = default;
};

extern int appMain(int, char **, IApplication *);
