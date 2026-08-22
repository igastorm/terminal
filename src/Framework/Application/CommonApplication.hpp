#pragma once
#include "IApplication.hpp"

class CommonApplication : public IApplication {
private:
  int ref_count = 0;

public:
  int addRef(void) override;
  int release(void) override;
  virtual ~CommonApplication(void) = default;

  static CommonApplication* init(void);
  static int startApp(int, char **);
};
