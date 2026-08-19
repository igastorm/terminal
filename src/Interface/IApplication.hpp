#pragma once
#include "IPTY.hpp"
#include "IWindow.hpp"

class IApplication : public IObject {
public:
  virtual IPTY *createPTY(void) = 0;
  virtual IWindow *createWindow(int, int, const char *) = 0;

  virtual ~IApplication(void) = default;
};

extern "C" int appMain(int, char **, IApplication *);
