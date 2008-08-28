#include "util.h"
#include <QString>
#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QFile>
#include <QtDebug>

void LeechCraft::Util::InstallTranslator (const QString& baseName)
{
	QTranslator *transl = new QTranslator;
	QString localeName = QString(::getenv ("LANG"));
	if (localeName.isEmpty ())
		localeName = QLocale::system ().name ();
	localeName = localeName.left (5);

	QString filename = QString (":/leechcraft_");
	if (!baseName.isEmpty ())
		filename.append (baseName).append ("_");
	filename.append (localeName);

	if (!transl->load (filename))
	{
		qWarning () << Q_FUNC_INFO << "could not load translation file for locale" << localeName << filename;
		return;
	}
	qApp->installTranslator (transl);
}

