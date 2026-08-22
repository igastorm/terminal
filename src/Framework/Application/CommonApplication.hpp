#pragma once
#include "IApplication.hpp"

class CommonApplication : public IApplication {
private:
  int ref_count = 0;

protected:
  int addRef(void) override;
  int release(void) override;
  virtual ~CommonApplication(void) = default;

  static CommonApplication *init(void);

public:
  static int startApp(int, char **);
};
