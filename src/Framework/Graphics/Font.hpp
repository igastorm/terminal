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

public:
  FontCellSize getCellSize() const override;
  int release() override;
  Font(float, float);
  ~Font();
};
