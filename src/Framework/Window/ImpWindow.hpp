#include "../Application/ImpApplication.hpp"
#include "CommonWindow.hpp"

template <class PlatformData> class ImpWindow : public CommonWindow {
private:
  PlatformData data;

  bool setTitle(const char *) override;
  bool show() override;
  bool hide() override;

public:
  template<class PlatformApplicationData>
  static ImpWindow *createWindow(ImpApplication<PlatformApplicationData> *, int, int, const char *);
  virtual ~ImpWindow() override;
};

