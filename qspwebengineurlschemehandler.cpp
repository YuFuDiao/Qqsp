#include "qspwebengineurlschemehandler.h"

#include "comtools.h"

#include <QBuffer>
#include <QFileInfo>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QMultiMap>
#include <QUrl>

QspWebEngineUrlSchemeHandler::QspWebEngineUrlSchemeHandler(QObject* parent)
	: QWebEngineUrlSchemeHandler(parent)
{
}

// Chromium only hands a resource to its media pipeline when the response carries
// a media content type; some Windows MIME databases report application/octet-stream
// (or application/mp4) for video files and <video> then silently stays blank, so
// map the suffixes QSP games actually use.
static QByteArray contentTypeForPath(const QString& filePath, const QByteArray& detected)
{
	if (detected.startsWith("video/") || detected.startsWith("audio/")) {
		return detected;
	}
	const QString suffix = QFileInfo(filePath).suffix().toLower();
	if (suffix == QLatin1String("mp4") || suffix == QLatin1String("m4v")) {
		return QByteArrayLiteral("video/mp4");
	}
	if (suffix == QLatin1String("webm")) {
		return QByteArrayLiteral("video/webm");
	}
	if (suffix == QLatin1String("mkv")) {
		return QByteArrayLiteral("video/x-matroska");
	}
	if (suffix == QLatin1String("ogv")) {
		return QByteArrayLiteral("video/ogg");
	}
	if (suffix == QLatin1String("avi")) {
		return QByteArrayLiteral("video/x-msvideo");
	}
	if (suffix == QLatin1String("mov")) {
		return QByteArrayLiteral("video/quicktime");
	}
	if (suffix == QLatin1String("wmv")) {
		return QByteArrayLiteral("video/x-ms-wmv");
	}
	if (suffix == QLatin1String("mp3")) {
		return QByteArrayLiteral("audio/mpeg");
	}
	if (suffix == QLatin1String("m4a")) {
		return QByteArrayLiteral("audio/mp4");
	}
	return detected;
}

// Chromium streams media using HTTP range requests. Answering with the whole file
// (the pre-Qt6 behaviour) leaves <video> unable to buffer, seek or loop, since a
// 200 response without ranges is not treated as a seekable media source. Reply
// with the requested range plus the matching Content-Range / Accept-Ranges
// headers; setAdditionalResponseHeaders() exists since Qt 6.6.
static bool parseByteRange(const QMap<QByteArray, QByteArray>& headers, qint64 totalSize,
                           qint64* outStart, qint64* outEnd)
{
	for (auto it = headers.constBegin(); it != headers.constEnd(); ++it) {
		if (it.key().compare(QByteArrayLiteral("Range"), Qt::CaseInsensitive) != 0) {
			continue;
		}
		QByteArray value = it.value().trimmed();
		if (!value.startsWith(QByteArrayLiteral("bytes="))) {
			continue;
		}
		value = value.mid(6);
		const int dash = value.indexOf(char(45));
		if (dash < 0) {
			continue;
		}
		const QByteArray firstPart = value.left(dash);
		const QByteArray lastPart = value.mid(dash + 1);
		bool okStart = false;
		bool okEnd = false;
		qint64 start = firstPart.isEmpty() ? 0 : firstPart.toLongLong(&okStart);
		qint64 end = lastPart.isEmpty() ? (totalSize - 1) : lastPart.toLongLong(&okEnd);
		if (!firstPart.isEmpty() && !okStart) {
			continue;
		}
		if (!lastPart.isEmpty() && !okEnd) {
			continue;
		}
		if (end >= totalSize) {
			end = totalSize - 1;
		}
		if (start > end || start >= totalSize) {
			continue;
		}
		*outStart = start;
		*outEnd = end;
		return true;
	}
	return false;
}
void QspWebEngineUrlSchemeHandler::requestStarted(QWebEngineUrlRequestJob* request)
{
	const QUrl url = request->requestUrl();
	QString url_str = url.toString();
	QBuffer* buffer = new QBuffer;
	connect(request, &QWebEngineUrlRequestJob::destroyed, buffer, &QBuffer::deleteLater);

	buffer->open(QIODevice::WriteOnly);
	if (url_str.compare("qsp:", Qt::CaseInsensitive) == 0 || url_str.compare("qsp:/", Qt::CaseInsensitive) == 0) {
		if (m_isUseHtml) {
			QString replystr;
			replystr.append("<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"UTF-8\">\n<style>\nbody {\n");
			if (m_backColor.isValid()) {
				replystr.append(QString("background-color: %1;\n").arg(m_backColor.name()));
			}
			if (m_fontColor.isValid()) {
				replystr.append(QString("color: %1;\n").arg(m_fontColor.name()));
			}
			if (m_fontType == 1) { // set in qsp
				replystr.append(QString("font-family: %1;\n").arg(m_font.family()));
			}
			if (m_fontType == 2) { // set by user
				replystr.append(QString("font-family: %1 !important;\n").arg(m_font.family()));
			}
			if (m_sizeType == 1) { // set in qsp
				replystr.append(QString("font-size: %1pt;\n").arg(m_font.pointSize()));
			}
			if (m_sizeType == 2) { // set by user
				replystr.append(QString("font-size: %1pt !important;\n").arg(m_font.pointSize()));
			}
			if (!m_bmpBg.isEmpty()) {
				replystr.append(QString("background: url(%1) no-repeat center center fixed;\nbackground-size: cover;\n").arg(m_bmpBg));
			}
			if (m_linkColor.isValid()) {
				replystr.append(QString("}\na:link {\ncolor: %1;\n").arg(m_linkColor.name()));
			}
			replystr.append("}\n</style>\n");
			if (m_isCustomCSS) {
				replystr.append("<link rel=\"stylesheet\" type=\"text/css\" href=\"custom.css\">\n");
			}
			replystr.append("<script src=\"qrc:///qtwebchannel/qwebchannel.js\"></script>\n");
			replystr.append(
				"<script type=\"text/javascript\"> \nvar qsp; \ndocument.addEventListener(\"DOMContentLoaded\", function () { \nnew "
				"QWebChannel(qt.webChannelTransport, function(channel) { \nqsp = channel.objects.qsp; \n}); \n});\n</script>\n");
			replystr.append(m_head);
			replystr.append("</head>\n<body>\n");
			replystr.append(m_text);
			replystr.append("</body>\n</html>");

			buffer->write(replystr.toUtf8());
			buffer->close();
			request->reply("text/html", buffer);
		} else {
			buffer->write(m_text.toUtf8());
			buffer->close();
			request->reply("text/plain; charset=utf-8", buffer);
		}
	} else {
		QString tmpurl = url_str.mid(5);
		if (tmpurl.contains('#') && !tmpurl.isEmpty()) {
			tmpurl = tmpurl.split('#').at(0);
		}
		tmpurl = QByteArray::fromPercentEncoding(tmpurl.toUtf8());
		const QString path = QSPTools::GetCaseInsensitiveFilePath(m_path, tmpurl);
		const QString filePath = m_path + path;
		QMimeDatabase db;
		const QByteArray contentType =
			contentTypeForPath(filePath, db.mimeTypeForFile(filePath).name().toUtf8());

		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly)) {
			buffer->close();
			request->fail(QWebEngineUrlRequestJob::UrlNotFound);
			return;
		}
		const qint64 totalSize = file.size();
		qint64 rangeStart = 0;
		qint64 rangeEnd = totalSize > 0 ? totalSize - 1 : 0;
		const bool hasRange =
			parseByteRange(request->requestHeaders(), totalSize, &rangeStart, &rangeEnd);

		QMultiMap<QByteArray, QByteArray> extraHeaders;
		extraHeaders.insert(QByteArrayLiteral("Accept-Ranges"), QByteArrayLiteral("bytes"));
		if (hasRange) {
			QByteArray contentRange = QByteArrayLiteral("bytes ");
			contentRange += QByteArray::number(rangeStart);
			contentRange += QByteArrayLiteral("-");
			contentRange += QByteArray::number(rangeEnd);
			contentRange += QByteArrayLiteral("/");
			contentRange += QByteArray::number(totalSize);
			extraHeaders.insert(QByteArrayLiteral("Content-Range"), contentRange);
		}
		request->setAdditionalResponseHeaders(extraHeaders);

		if (file.seek(rangeStart)) {
			buffer->write(file.read(rangeEnd - rangeStart + 1));
		}
		buffer->close();
		file.close();
		request->reply(contentType, buffer);
	}
}

void QspWebEngineUrlSchemeHandler::SetPlainText(const QString& text)
{
	m_isUseHtml = false;
	m_text = text;
}

void QspWebEngineUrlSchemeHandler::SetHtml(const QString& text)
{
	m_isUseHtml = true;
	m_text = text;
}

void QspWebEngineUrlSchemeHandler::SetGamePath(const QString& path)
{
	m_path = path;
}

void QspWebEngineUrlSchemeHandler::SetLinkColor(const QColor& color)
{
	m_linkColor = color;
}

void QspWebEngineUrlSchemeHandler::SetBackgroundColor(const QColor& color)
{
	m_backColor = color;
}

void QspWebEngineUrlSchemeHandler::SetForegroundColor(const QColor& color)
{
	m_fontColor = color;
}

void QspWebEngineUrlSchemeHandler::SetBackgroundImage(const QString& path)
{
	m_bmpBg = path;
}

void QspWebEngineUrlSchemeHandler::SetTextFont(const QFont& new_font)
{
	m_font = new_font;
}

void QspWebEngineUrlSchemeHandler::SetHead(const QString& head)
{
	m_head = head;
}

void QspWebEngineUrlSchemeHandler::SetFontType(const int fontType)
{
	m_fontType = fontType;
}

void QspWebEngineUrlSchemeHandler::SetSizeType(const int sizeType)
{
	m_sizeType = sizeType;
}

void QspWebEngineUrlSchemeHandler::SetCustomCSS(bool customCSS)
{
	m_isCustomCSS = customCSS;
}
