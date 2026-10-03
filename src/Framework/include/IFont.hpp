#pragma once
#include "IObject.hpp"

struct FontCellSize {
  float width = 0.0f;
  float height = 0.0f;
  float ascent = 0.0f;
  float descent = 0.0f;
  float leading = 0.0f;
};

class IFont : public IObject {
  public:
    virtual FontCellSize getCellSize() const;
    static IFont* createFont(const char8_t*, float);
    virtual ~IFont() = default;
};
