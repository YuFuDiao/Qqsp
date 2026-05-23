#include "comtools.h"
#include "consts.hpp"
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
	using namespace Qt::StringLiterals;

	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

#ifdef QT_WEBENGINEWIDGETS_LIB
	register_url_schemes();
#endif

	QApplication a(argc, argv);
	QCoreApplication::setApplicationName(Qqsp::appname);
	QCoreApplication::setOrganizationName("Qqsp");
	QCoreApplication::setApplicationVersion(VERSION_STRING);

	QString langid;
	QFileInfo settingsFile(QApplication::applicationDirPath() + '/'_L1 + Qqsp::customConfig);
	if (settingsFile.exists() && settingsFile.isFile()) {
		QSettings settings(QApplication::applicationDirPath() + '/'_L1 + Qqsp::customConfig, QSettings::IniFormat);
		langid = settings.value("application/language", Qqsp::SettingsValues::Application::langidSystem).toString();
	} else {
		QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
		langid = settings.value("application/language", Qqsp::SettingsValues::Application::langidSystem).toString();
	}

	QTranslator qspTranslator;

	if (langid == Qqsp::SettingsValues::Application::langidSystem) {
		if (QString preferredQmFile = QSPTools::selectAvailableTranslation({QCoreApplication::applicationDirPath(), ":/i18n"_L1}, QLocale{}.uiLanguages());
		    !preferredQmFile.isEmpty()) {
			if (qspTranslator.load(preferredQmFile)) {
				QCoreApplication::installTranslator(&qspTranslator);
			}
		}
	} else {
		if (qspTranslator.load(QApplication::applicationName() + '.'_L1 + langid, QApplication::applicationDirPath())) {
			QCoreApplication::installTranslator(&qspTranslator);
		} else if (qspTranslator.load(QApplication::applicationName() + '.'_L1 + langid, ":/i18n/"_L1)) {
			QCoreApplication::installTranslator(&qspTranslator);
		}
	}

	QCommandLineParser parser;
	parser.setApplicationDescription("Qqsp"_L1);
	parser.addPositionalArgument("file"_L1, QCoreApplication::translate("main", "Game file to open."));
	parser.addHelpOption();
	parser.addVersionOption();
	parser.process(a);

	MainWindow w;
	w.SetLangID(langid);

	if (parser.positionalArguments().size() != 0) {
		QFileInfo file(parser.positionalArguments().at(0));
		w.OpenGameFile(file.filePath());
	}

	w.show();

	return a.exec();
}
