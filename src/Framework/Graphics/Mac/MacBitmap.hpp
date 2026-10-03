#pragma once
#include "../CommonGraphics.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

struct FontRasterizerData {
  CGContextRef cg_context = nullptr;
  void* bitmap_data = nullptr;
};

class MacBitmap : public CommonBitmap {
private:
  FontRasterizerData data = {};
  MacBitmap(CGContextRef, void*);

public:
  static MacBitmap *createMacBitmap(std::size_t, std::size_t, std::size_t,
                                    std::size_t);
  MacBitmap() = delete;
  ~MacBitmap();
};
