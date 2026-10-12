#include "qspwebengineurlschemehandler.h"

#include "comtools.h"
#include "diaglog.h"

#include <QBuffer>
#include <QFileInfo>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QSet>
#include <QMap>
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

void QspWebEngineUrlSchemeHandler::requestStarted(QWebEngineUrlRequestJob* request)
{
	qqspDiag(QStringLiteral("REQ %1").arg(request->requestUrl().toString()));
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
			qqspDiag(QStringLiteral("PAGE html bodyLen=%1").arg(m_text.length()));
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
		const QFileInfo fileInfo(filePath);
		if (!fileInfo.exists() || !fileInfo.isFile()) {
			buffer->close();
			qqspDiag(QStringLiteral("NOTFOUND %1").arg(filePath));
		request->fail(QWebEngineUrlRequestJob::UrlNotFound);
			return;
		}


		QMimeDatabase db;
		const QByteArray contentType =
			contentTypeForPath(filePath, db.mimeTypeForFile(filePath).name().toUtf8());
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly)) {
			buffer->close();
			qqspDiag(QStringLiteral("NOTFOUND %1").arg(filePath));
		request->fail(QWebEngineUrlRequestJob::UrlNotFound);
			return;
		}
		// Serve the media bytes through the scheme handler.  A redirect to file:// is a
		// cross-scheme navigation that Chromium refuses, and a plain 200 without range
		// support is not treated as a seekable media source, which makes the element
		// fail with MEDIA_ERR_SRC_NOT_SUPPORTED.  Honour the Range header in place and
		// advertise range support, so Chromium can stream, seek and loop the video.
		const qint64 totalSize = file.size();
		qint64 rangeStart = 0;
		qint64 rangeEnd = totalSize > 0 ? totalSize - 1 : 0;
		bool hasRange = false;
		const QMap<QByteArray, QByteArray> reqHeaders = request->requestHeaders();
		for (auto it = reqHeaders.constBegin(); it != reqHeaders.constEnd(); ++it) {
			if (it.key().compare(QByteArrayLiteral("Range"), Qt::CaseInsensitive) != 0) {
				continue;
			}
			QByteArray rv = it.value().trimmed();
			if (!rv.startsWith(QByteArrayLiteral("bytes="))) {
				continue;
			}
			rv = rv.mid(6);
			const int dash = rv.indexOf(char(45));
			if (dash < 0) {
				continue;
			}
			const QByteArray ra = rv.left(dash);
			const QByteArray rb = rv.mid(dash + 1);
			bool oka = false;
			bool okb = false;
			qint64 rs = ra.isEmpty() ? 0 : ra.toLongLong(&oka);
			qint64 re = rb.isEmpty() ? (totalSize - 1) : rb.toLongLong(&okb);
			if ((!ra.isEmpty() && !oka) || (!rb.isEmpty() && !okb)) {
				continue;
			}
			if (re >= totalSize) {
				re = totalSize - 1;
			}
			if (rs > re || rs >= totalSize) {
				continue;
			}
			rangeStart = rs;
			rangeEnd = re;
			hasRange = true;
			break;
		}
		QMultiMap<QByteArray, QByteArray> extra;
		extra.insert(QByteArrayLiteral("Accept-Ranges"), QByteArrayLiteral("bytes"));
		if (hasRange) {
			QByteArray cr = QByteArrayLiteral("bytes ");
			cr += QByteArray::number(rangeStart);
			cr += QByteArrayLiteral("-");
			cr += QByteArray::number(rangeEnd);
			cr += QByteArrayLiteral("/");
			cr += QByteArray::number(totalSize);
			extra.insert(QByteArrayLiteral("Content-Range"), cr);
		}
		request->setAdditionalResponseHeaders(extra);
		if (file.seek(rangeStart)) {
			buffer->write(file.read(rangeEnd - rangeStart + 1));
		} else {
			buffer->write(file.readAll());
		}
		qqspDiag(QStringLiteral("SERVE %1 type=%2 range=%3 total=%4")
			.arg(filePath).arg(QString::fromUtf8(contentType)).arg(hasRange ? 1 : 0).arg(totalSize));
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
		qqspDiag(QStringLiteral("SetHtml len=%1").arg(text.length()));
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
