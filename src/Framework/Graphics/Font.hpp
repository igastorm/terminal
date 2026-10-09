#pragma once
#include "../Object/Object.hpp"
#include "IFont.hpp"

//  ========================================================
//
//  Font
//
//  ========================================================

class Font : public Object<IFont> {
private:
  const float half_width = 0.0f;
  const float height = 0.0f;

protected:
  Font(float, float);
  ~Font() = default;

public:
  FontCellSize getCellSize() const override;
  int release() override;
};
