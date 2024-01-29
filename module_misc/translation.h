#ifndef TRANSLATION_H
#define TRANSLATION_H

#include <QTranslator>
#include <QString>

namespace MiscTools
{

class Translation
{
    Translation();
public:
    static QTranslator* getTranslator(const QString& locale,
                                      const QStringList& projPaths,
                                      const QString& filename);

    static void setDefaultLocale(const QString& locale);
};

} // namespace MiscTools

#endif // TRANSLATION_H
