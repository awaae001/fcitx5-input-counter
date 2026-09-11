// SPDX-License-Identifier: MIT

//! Entry point of the input statistics viewer.

#include <QApplication>
#include <QIcon>
#include <QString>
#include <clocale>

#include <fcitx-utils/i18n.h>

#include "main_window.h"

int main(int argc, char *argv[]) {
  std::setlocale(LC_ALL, "");
  fcitx::registerDomain("fcitx5-input-counter", INPUT_COUNTER_LOCALEDIR);

  QApplication app(argc, argv);
  QApplication::setApplicationName(
      QStringLiteral("fcitx5-input-counter-viewer"));
  QApplication::setApplicationDisplayName(QString(_("Input Counter")));
  // Wayland has no per-window icon: the compositor resolves the icon from
  // the desktop file matching this app id.
  QApplication::setDesktopFileName(
      QStringLiteral("fcitx5-input-counter-viewer"));

  inputcounter::MainWindow window;
  // X11 fallback; on Wayland this is ignored in favor of the desktop file.
  window.setWindowIcon(
      QIcon::fromTheme(QStringLiteral("fcitx5-input-counter")));
  window.show();
  return QApplication::exec();
}
