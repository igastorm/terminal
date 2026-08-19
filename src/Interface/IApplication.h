#pragma once
#include "IPTY.h"
#include "IWindow.h"

class IApplication : public IObject {
public:
  virtual IPTY *createPTY(void) = 0;
  virtual IWindow *createWindow(int, int, const char *) = 0;

  virtual ~IApplication(void) = default;
};

int appMain(int, char **, IApplication *);
