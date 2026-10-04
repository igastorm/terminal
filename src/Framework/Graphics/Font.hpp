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
  const float size = 0.0f;

public:
  int addRef() override;
  int release() override;
  Font(float);
  ~Font();
};