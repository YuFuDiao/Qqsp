#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>
#include <QLocale>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QTranslator>

#ifdef QT_WEBENGINEWIDGETS_LIB
#	include "url_schemes.h"
#endif

#include "qqsp-config.h"

int main(int argc, char* argv[])
{
	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

#ifdef QT_WEBENGINEWIDGETS_LIB
	register_url_schemes();
#endif

	QApplication a(argc, argv);
	a.setApplicationName("Qqsp");
	a.setOrganizationName("Qqsp");
	a.setApplicationVersion(VERSION_STRING);

	QObject::tr("__LANGNAME__");
	QObject::tr("__LANGID__");

	QString langid;
	QFileInfo settingsFile(QApplication::applicationDirPath() + "/" + QSP_CUSTOM_CONFIG);
	if (settingsFile.exists() && settingsFile.isFile()) {
		QSettings settings(QApplication::applicationDirPath() + "/" + QSP_CUSTOM_CONFIG, QSettings::IniFormat);
		langid = settings.value("application/language", QLocale::system().name()).toString();
	} else {
		QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
		langid = settings.value("application/language", QLocale::system().name()).toString();
	}

	QTranslator qspTranslator;

	if (qspTranslator.load(QApplication::applicationName() + "." + langid, QApplication::applicationDirPath())) {
		a.installTranslator(&qspTranslator);
	} else if (qspTranslator.load(QApplication::applicationName() + "." + langid, QLatin1String(":/i18n/"))) {
		a.installTranslator(&qspTranslator);
	}

	QTranslator qtTranslator;
	if (qtTranslator.load(QLatin1String("qtbase_%1").arg(langid.left(2)), QLatin1String(":/i18n/"))) {
		a.installTranslator(&qtTranslator);
	}

	QCommandLineParser parser;
	parser.setApplicationDescription("Qqsp");
	parser.addPositionalArgument("file", QCoreApplication::translate("main", "Game file to open."));
	parser.addHelpOption();
	parser.addVersionOption();
	parser.process(a);

	MainWindow w;

	if (parser.positionalArguments().size() != 0) {
		QFileInfo file(parser.positionalArguments().at(0));
		w.OpenGameFile(file.filePath());
	}

	w.show();

	return a.exec();
}
