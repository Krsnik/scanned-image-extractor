#include <QLocale>
// #include <QForeachContainer>
#include <QDebug>
#include <QStringList>

#include "translation.h"

namespace MiscTools
{

Translation::Translation()
{
}

void Translation::setDefaultLocale(const QString& locale)
{
    QString shortLocale = locale.left(2).toLower();
    if (shortLocale == "de") {
        QLocale::setDefault(QLocale::German);
    } else {
        QLocale::setDefault(QLocale::English);
    }
}

QTranslator* Translation::getTranslator(const QString& locale,
                                        const QStringList& projPaths,
                                        const QString& filename)
{
    QTranslator* translator = new QTranslator();
    QString shortLocale = locale.left(2).toLower();
    bool found = false;
    // first search in given paths
    foreach(const QString& path, projPaths) {
        if (translator->load(path + filename + locale)) {
            found = true;
            break;
        }
    }
    // if not found, try to load  short locale
    if (!found) {
        if (shortLocale == "en") {
            // this is english, anyways
        } else {
            qDebug() << "now trying to load " << shortLocale
                     << "for" << filename;
            // try to load short locale in the given paths
            foreach(const QString& path, projPaths) {
                if (translator->load(path + filename + shortLocale)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                // finally, fall back to english hardcoded
                qDebug() << "could not load translation " << QString(shortLocale)
                         << "for" << filename;
                qDebug() << "falling back to hard-coded \"en\"";
            }
        }
    }
    return translator;
}

} // end namespace misc
