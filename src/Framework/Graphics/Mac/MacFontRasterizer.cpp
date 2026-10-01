#include "MacFontRasterizer.hpp"

MacFontRasterizer::MacFontRasterizer(IFont *font) : FontRasterizer(font) {}

MacFontRasterizer::~MacFontRasterizer() {}

MacFontRasterizer *MacFontRasterizer::createMacFontRasterizer(IFont *) {
  return nullptr;
}

IFontRasterizer *IFontRasterizer::createFontRasterizer(IFont *font) {
  return MacFontRasterizer::createMacFontRasterizer(font);
}
