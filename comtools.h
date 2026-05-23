#ifndef TOOLS_H
#define TOOLS_H

#include <qsp_default.h>

#include <QColor>
#include <QHash>
#include <QString>
#include <QStringList>

#include <vector>

class QSPTools {
public:
	static QString GetHexColor(const QColor color);
	static QString HtmlizeWhitespaces(const QString& str);
	static QString ProceedAsPlain(const QString& str);
	static QString GetAppPath();
	static QString GetCaseInsensitiveFilePath(QString searchDir, QString originalPath);
	static QString GetCaseInsensitiveAbsoluteFilePath(QString searchDir, QString originalPath);
	static QString qspStrToQt(const QSPString& str);
	static void qtStrToQspBuffer(const QString& str, QSP_CHAR* buffer, int bufLen);
	static QColor wxtoQColor(int wxColor);

	static std::vector<QSPListItem> qspActions();
	static std::vector<QSPObjectItem> qspObjects();

	static bool loadGameFile(QString path);
	static bool reloadGame(const QString& gameFilePath);

	static bool useCaseInsensitiveFilePath;

	static QStringList availableTranslations(const QString& directory);
	static QString selectAvailableTranslation(const QStringList& translationDirectories, const QStringList& userPreferences);

private:
	static QHash<QString, QString> file_list;
	static QString file_path;
};

constexpr QSPString operator""_qsp(const QSP_CHAR* str, std::size_t size)
{
	return {const_cast<QSP_CHAR*>(str), const_cast<QSP_CHAR*>(str) + size};
}

#endif
