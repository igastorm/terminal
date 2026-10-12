#pragma once
#include "IAppHandler.hpp"

class IApplication {
public:
  virtual bool run(const char8_t *, IAppHandler *) = 0;
  virtual void terminate() = 0;

  virtual void postEvent(void*) = 0;

  ~IApplication() = default;
};

extern int appMain(int, char **, IApplication *);
