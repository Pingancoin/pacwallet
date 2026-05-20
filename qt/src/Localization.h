#pragma once

#include <QString>

namespace pacqt::l10n {

QString defaultLanguageCode();
QString currentLanguageCode();
void setCurrentLanguageCode(const QString &code);
QString text(const QString &source);
QString languageDisplayName(const QString &code);

} // namespace pacqt::l10n
