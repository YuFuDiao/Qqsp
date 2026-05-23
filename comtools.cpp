#include "comtools.h"

#include "consts.hpp"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>

using namespace Qt::StringLiterals;

namespace {
	QString langId(const QString& qmFileName)
	{
		// qqsp.langid.qm
		return qmFileName.mid(Qqsp::appname.size() + 1, qmFileName.lastIndexOf('.'_L1) - Qqsp::appname.size() - 2);
	}
} // namespace

QHash<QString, QString> QSPTools::file_list;
QString QSPTools::file_path;
bool QSPTools::useCaseInsensitiveFilePath = true;

QString QSPTools::GetHexColor(const QColor color)
{
	return QString("%1%2%3").arg(color.red(), 2, 16, QLatin1Char('0')).arg(color.green(), 2, 16, QLatin1Char('0')).arg(color.blue(), 2, 16, QLatin1Char('0'));
}

QString QSPTools::HtmlizeWhitespaces(const QString& str)
{
	QString::const_iterator i;
	QChar ch, quote;
	QString out;
	size_t j, linepos = 0;
	bool isLastSpace = true;
	for (i = str.begin(); i != str.end(); ++i) {
		ch = *i;
		if (ch == QLatin1Char('<')) {
			quote = QLatin1Char(0);
			while (i != str.end()) {
				ch = *i;
				if (quote.unicode()) {
					if (ch == QLatin1Char('\\')) {
						if (++i == str.end()) {
							break;
						}
						ch = *i;
						if (ch == quote) {
							if (ch == QLatin1Char('"')) {
								out.append(QLatin1String("&quot;"));
							} else if (ch == QChar('\'')) {
								out.append(QLatin1String("&apos;"));
							}
							++i;
							continue;
						}
						out.append(QLatin1Char('\\'));
					}
					if (ch == QLatin1Char('&')) {
						out.append(QLatin1String("&amp;"));
					} else if (ch == QLatin1Char('\n')) {
						out.append(QLatin1String("%0A"));
					} else if (ch == QLatin1Char('<')) {
						out.append(QLatin1String("&lt;"));
					} else if (ch == QLatin1Char('>')) {
						out.append(QLatin1String("&gt;"));
					} else {
						if (ch == quote) {
							quote = QLatin1Char(0);
						}
						out.append(ch);
					}
				} else {
					out.append(ch);
					if (ch == QLatin1Char('>')) {
						break;
					} else if (ch == QLatin1Char('"') || ch == QLatin1Char('\'')) {
						quote = ch;
					}
				}
				++i;
			}
			if (i == str.end()) {
				return out;
			}
			isLastSpace = true;
		} else if (ch == QLatin1Char(' ')) {
			if (isLastSpace) {
				out.append(QLatin1String("&ensp;"));
			} else {
				out.append(QLatin1Char(' '));
			}
			isLastSpace = !isLastSpace;
			++linepos;
		} else if (ch == QLatin1Char('\r')) {
		} else if (ch == QChar('\n')) {
			out.append(QLatin1String("<br>"));
			isLastSpace = true;
			linepos = 0;
		} else if (ch == QLatin1Char('\t')) {
			for (j = 4 - linepos % 4; j > 0; --j) {
				if (isLastSpace) {
					out.append(QLatin1String("&emsp;"));
				} else {
					out.append(QLatin1Char(' '));
				}
				isLastSpace = !isLastSpace;
			}
			linepos += 4 - linepos % 4;
		} else {
			out.append(ch);
			isLastSpace = false;
			++linepos;
		}
	}
	return out;
}

QString QSPTools::ProceedAsPlain(const QString& str)
{
	QString::const_iterator i;
	QChar ch;
	QString out;
	for (i = str.begin(); i != str.end(); ++i) {
		ch = *i;
		if (ch == QLatin1Char('<')) {
			out.append(QLatin1String("&lt;"));
		} else if (ch == QLatin1Char('>')) {
			out.append(QLatin1String("&gt;"));
		} else if (ch == QLatin1Char('&')) {
			out.append(QLatin1String("&amp;"));
		} else {
			out.append(ch);
		}
	}
	return out;
}

QString QSPTools::GetAppPath()
{
	return QCoreApplication::applicationDirPath();
}

QString QSPTools::GetCaseInsensitiveFilePath(QString searchDir, QString originalPath)
{
	QString new_name = originalPath.replace("\\", "/");
	if (new_name.startsWith("/")) {
		new_name = new_name.remove(0, 1);
	}
#ifndef Q_OS_WIN
	if (useCaseInsensitiveFilePath) {
		QDir itDir(searchDir);
		if (file_path != searchDir && !searchDir.isEmpty()) {
			file_list.clear();
			QDirIterator it(searchDir, QDir::Files, QDirIterator::Subdirectories);
			while (it.hasNext()) {
				it.next();
				file_list.insert(itDir.relativeFilePath(it.filePath()).toLower(), itDir.relativeFilePath(it.filePath()));
			}
			file_path = searchDir;
		}
		if (file_list.contains(new_name.toLower())) {
			return itDir.relativeFilePath(file_list.value(new_name.toLower()));
		}
	}
#endif
	return new_name;
}

QString QSPTools::GetCaseInsensitiveAbsoluteFilePath(QString searchDir, QString originalPath)
{
	QString new_name = originalPath.replace("\\", "/");
#ifndef _WIN32
	if (useCaseInsensitiveFilePath) {
		QDir itDir(searchDir);
		if (originalPath.startsWith(searchDir)) {
			new_name = new_name.remove(0, searchDir.length());
		}
		if (file_path != searchDir && !searchDir.isEmpty()) {
			file_list.clear();
			QDirIterator it(searchDir, QDir::Files, QDirIterator::Subdirectories);
			while (it.hasNext()) {
				it.next();
				file_list.insert(itDir.relativeFilePath(it.filePath()).toLower(), itDir.relativeFilePath(it.filePath()));
			}
			file_path = searchDir;
		}
		if (file_list.contains(new_name.toLower())) {
			return itDir.absoluteFilePath(file_list.value(new_name.toLower()));
		}
	}
#endif
	return new_name;
}

QString QSPTools::qspStrToQt(const QSPString& str)
{
	return str.Str ? QString::fromWCharArray(str.Str, (int)(str.End - str.Str)) : QString{};
}

void QSPTools::qtStrToQspBuffer(const QString& str, QSP_CHAR* buffer, int bufLen)
{
	if (bufLen < 1) {
		return;
	}
#ifdef _UNICODE
	const auto stdStr = str.toStdWString();
#else
	const auto stdStr = str.toStdString();
#endif
	static_assert(sizeof(decltype(stdStr)::value_type) == sizeof(QSP_CHAR));

	int charsToCopy = qMin(str.size() + 1, bufLen);
	std::memcpy(buffer, stdStr.c_str(), sizeof(QSP_CHAR) * charsToCopy);
}

QColor QSPTools::wxtoQColor(int wxColor)
{
	QColor col;
	if (wxColor == 0) {
		col = Qt::black;
		return col;
	}
	col = QColor::fromRgba(wxColor);
	int red = col.red();
	col.setRed(col.blue());
	col.setBlue(red);
	return col;
}

namespace {
	template <typename Item, typename Getter = int(Item* items, int bufferSize)>
	std::vector<Item> listQspItems(Getter getter)
	{
		int count = getter(nullptr, 0);
		std::vector<Item> res(count);
		getter(res.data(), res.size());
		return res;
	}
} // namespace

std::vector<QSPListItem> QSPTools::qspActions()
{
	return listQspItems<QSPListItem>(QSPGetActions);
}

std::vector<QSPObjectItem> QSPTools::qspObjects()
{
	return listQspItems<QSPObjectItem>(QSPGetObjects);
}

bool QSPTools::loadGameFile(QString path)
{
	QFile inp{path};
	if (!inp.open(QIODevice::ReadOnly)) {
		return false;
	}
	const auto fileSize = static_cast<int>(inp.size());
	QByteArray data{fileSize + 3, Qt::Uninitialized};
	inp.read(data.data(), inp.size());
	data[fileSize] = data[fileSize + 1] = data[fileSize + 2] = 0;
	return QSPLoadGameWorldFromData(data.data(), data.size(), QSP_TRUE);
}

bool QSPTools::reloadGame(const QString& gameFilePath)
{
	int requiredBufSize = 0;
	QSPSaveGameAsData(nullptr, &requiredBufSize, QSP_FALSE);
	QByteArray buf{requiredBufSize, Qt::Uninitialized};
	if (!QSPSaveGameAsData(buf.data(), &requiredBufSize, QSP_FALSE)) {
		return false;
	}
	if (!loadGameFile(gameFilePath)) {
		return false;
	}
	return QSPOpenSavedGameFromData(buf.data(), requiredBufSize, QSP_TRUE) == QSP_TRUE;
}

QStringList QSPTools::availableTranslations(const QString& directory)
{
	QStringList result;
	QDirIterator qmItRC(directory, QStringList() << (Qqsp::appname + ".*.qm"_L1), QDir::Files);
	while (qmItRC.hasNext()) {
		qmItRC.next();
		QFileInfo finfo = qmItRC.fileInfo();
		result.push_back(langId(finfo.completeBaseName()));
	}
	return result;
}

QString QSPTools::selectAvailableTranslation(const QStringList& translationDirectories, const QStringList& userPreferences)
{
	for (const QString& directory: translationDirectories) {
		const QStringList available{availableTranslations(directory)};
		for (const QString& preferred: userPreferences) {
			if (available.contains(preferred)) {
				return "%1/%2.%3.qm"_L1.arg(directory, Qqsp::appname, preferred);
			}
		}
	}
	return {};
}
