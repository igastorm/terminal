#include "../Application/ImpApplication.hpp"
#include "../Application/MacApplication.h"

template <>
IGraphicsDevice *
ImpApplication<ImpApplicationData>::createGraphicsDevice() {
  return nullptr;
}
