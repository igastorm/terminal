#pragma once
#include "IApplication.hpp"

class Application : public IApplication {
protected:
  IAppHandler *handler = nullptr;

  Application() = default;

  virtual ~Application() = default;
};
