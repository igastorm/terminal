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

public:
  int addRef() override;
  int release() override;
  Font();
  ~Font();
};