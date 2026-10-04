#pragma once
#include "IFont.hpp"

//  ========================================================
//
//  Font
//
//  ========================================================

class Font : public IFont {
private:
  int ref_count = 0;
  const float half_width = 0.0f;
  const float height = 0.0f;

public:
  FontCellSize getCellSize() const override;
  int addRef() override;
  int release() override;
  Font(float, float);
  ~Font();
};