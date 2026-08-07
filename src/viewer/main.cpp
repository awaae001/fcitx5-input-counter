// SPDX-License-Identifier: MIT

//! Entry point of the input statistics viewer.

#include <clocale>

#include <QApplication>

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

  inputcounter::MainWindow window;
  window.show();
  return QApplication::exec();
}
