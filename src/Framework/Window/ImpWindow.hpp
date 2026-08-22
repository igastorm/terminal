#include "../Application/ImpApplication.hpp"
#include "CommonWindow.hpp"

template <class platformData> class ImpWindow : public CommonWindow {
private:
  platformData data;

  bool setTitle(const char *) override;
  bool show(void) override;
  bool hide(void) override;

public:
  static ImpWindow *createWindow(ImpApplication *, int, int, const char *);
  virtual ~ImpWindow(void) override;
};

