#pragma once
#include "IBitmap.hpp"

//  ========================================================
//
//  Bitmap
//
//  ========================================================

class Bitmap : public IBitmap {
private:
  int ref_count = 0;
  const std::size_t width = 0;
  const std::size_t height = 0;
  void *bitmap_data = nullptr;

public:
  [[nodiscard]] void *getBitmapData() const override;
  [[nodiscard]] std::size_t getWidth() const override;
  [[nodiscard]] std::size_t getHeight() const override;
  int addRef() override;
  int release() override;
  Bitmap(std::size_t, std::size_t, void *);
  ~Bitmap();
};