#pragma once
#include "../include/ITerminal.hpp"
#include "IGraphicsDevice.hpp"
#include "ISurface.hpp"
#include "IWindow.hpp"

class Terminal : public ITerminal {
private:
  IWindow *window = nullptr;
  IGraphicsDevice *device = nullptr;
  ISurface *swap_chain = nullptr;

  bool onInit(IApplication *) override;
  AppResult onEvent(IApplication *, const Event &) override;
  void onQuit(IApplication *app) override;

  bool createTerminalWindow();
  bool destroyTerminalWindow();

public:
};
