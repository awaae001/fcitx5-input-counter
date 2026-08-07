// SPDX-License-Identifier: MIT

//! Entry point of the input statistics viewer.

#include <clocale>

#include <QApplication>
#include <QIcon>

#include "i18n.h"
#include "main_window.h"

int main(int argc, char *argv[]) {
  std::setlocale(LC_ALL, "");
  bindtextdomain("fcitx5-input-counter", INPUT_COUNTER_LOCALEDIR);
  bind_textdomain_codeset("fcitx5-input-counter", "UTF-8");
  // gettext() looks up the default domain; without this the translations
  // above are never consulted.
  textdomain("fcitx5-input-counter");

  QApplication app(argc, argv);
  QApplication::setApplicationName(
      QStringLiteral("fcitx5-input-counter-viewer"));
  QApplication::setApplicationDisplayName(QString(IC_("Input statistics")));
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
