#pragma once
#include "IAppHandler.hpp"

class ITerminal : public IAppHandler {
public:
  [[nodiscard]] static ITerminal *getInstance();
  ~ITerminal() = default;
};
