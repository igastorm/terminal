#include "Application.hpp"
#include <clocale>
#include <cstdio>
#include <cstdlib>

int Application::addRef() { return ++this->ref_count; }

int Application::release() {
  if (--this->ref_count == 0) {
    this->~Application();
    free(this);
    return 0;
  }
  return this->ref_count;
}

Application::Application() { this->addRef(); }

int Application::startApp(int argc, char **argv) {
  Application *appInstance = createPlatformApplication();
  if (appInstance == nullptr) {
    std::perror("Failed to initialize appInstance");
    return 1;
  }
  int ret = appMain(argc, argv, appInstance);
  appInstance->release();
  return ret;
}

int main(int argc, char **argv) {
  std::setlocale(LC_ALL, "");
  return Application::startApp(argc, argv);
}
