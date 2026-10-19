#pragma once

class QCoreApplication;

namespace I18n {
// English is built in; Russian is installed when the system language is Russian
// (or SOTTO_LANG=ru). SOTTO_LANG=en forces English.
void install(QCoreApplication &app);
} // namespace I18n
