#pragma once

class QCoreApplication;

namespace I18n {
// English is built in; Russian is installed when the interface language setting says so, or when it
// follows the system and the system language is Russian. SOTTO_LANG=ru or =en overrides both.
void install(QCoreApplication &app);
} // namespace I18n
