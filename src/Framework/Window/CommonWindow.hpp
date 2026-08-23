#pragma once
#include "IWindow.hpp"

class CommonWindow : public IWindow {
private:
  int ref_count = 0;

public:
  inline static const char *fkeys[] = {
      "\033OP",   // F1
      "\033OQ",   // F2
      "\033OR",   // F3
      "\033OS",   // F4
      "\033[15~", // F5
      "\033[17~", // F6
      "\033[18~", // F7
      "\033[19~", // F8
      "\033[20~", // F9
      "\033[21~", // F10
      "\033[23~", // F11
      "\033[24~", // F12
  };

  int addRef() override;
  int release() override;
  virtual ~CommonWindow() = default;
};
