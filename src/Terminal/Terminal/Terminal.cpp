#include "Terminal.hpp"
#include <iostream>
using std::cout;

bool Terminal::onInit(IApplication *) {
  cout << "onIntit\n";
  return Terminal::createTerminalWindow();
}

AppResult Terminal::onEvent(IApplication *app, const Event &event) {
  if (event.type == EventType::WindowCloseRequest) {
    return this->destroyTerminalWindow() ? AppResult::Continue
                                         : AppResult::Quit;
  }

  if (event.type == EventType::AppReopen) {
    return this->createTerminalWindow() ? AppResult::Continue : AppResult::Quit;
  }

  return AppResult::Continue;
}

void Terminal::onQuit(IApplication *) {
  cout << "onQuit\n";
  if (this->window != nullptr) {
    this->destroyTerminalWindow();
  }
}

bool Terminal::createTerminalWindow() {
  cout << "createTerminalWindow\n";
  if (this->window != nullptr) {
    cout << "Window is already exist\n";
    return false;
  }

  cout << "createWindow\n";
  this->window = IWindow::createWindow(800, 600, "Terminal");

  if (this->window == nullptr) {
    cout << "Error: createWindow\n";
    return false;
  }

  return true;
}

bool Terminal::destroyTerminalWindow() {
  cout << "destroyTerminalWindow\n";
  if (this->window == nullptr) {
    cout << "Error: No window\n";
    return false;
  }

  this->window->release();
  this->window = nullptr;

  return true;
}

ITerminal *ITerminal::getInstance() {
  static Terminal instance;
  return &instance;
}
