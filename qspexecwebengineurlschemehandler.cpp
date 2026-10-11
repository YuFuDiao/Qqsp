#include "qspexecwebengineurlschemehandler.h"

#include "callbacks_gui.h"
#include "comtools.h"
#include "qspstr.h"

#include <qsp_default.h>

#include <QBuffer>
#include <QMessageBox>
#include <QString>
#include <QUrl>

QspExecWebEngineUrlSchemeHandler::QspExecWebEngineUrlSchemeHandler(QObject* parent)
	: QWebEngineUrlSchemeHandler(parent)
{
}

void QspExecWebEngineUrlSchemeHandler::requestStarted(QWebEngineUrlRequestJob* request)
{
	url = request->requestUrl();
	QTimer::singleShot(0, this, &QspExecWebEngineUrlSchemeHandler::QspLinkClicked);
}

void QspExecWebEngineUrlSchemeHandler::QspLinkClicked()
{
	emit qspLinkClicked(url);
}

void QspExecWebEngineUrlSchemeHandler::legacyLinkClicked(QWebEngineUrlRequestJob* request)
{
	url = request->requestUrl();
	QString href;
	href = QByteArray::fromPercentEncoding(url.toString().toUtf8());
	QString string = href.mid(5);
	if (!QSPExecString(QSPStr(string), QSP_TRUE)) {
		QString errorMessage;
		const QSPErrorInfo errorInfo{QSPGetLastErrorData()};
		QString desc = QSPTools::qspStrToQt(errorInfo.ErrorDesc);
		if (errorInfo.LocName.Str) {
			errorMessage = QString("位置: %1\n区域: %2\n行号: %3\n代码: %4\n描述: %5")
			                   .arg(QSPTools::qspStrToQt(errorInfo.LocName))
			                   .arg(errorInfo.ActIndex < 0 ? QString("访问时") : QString("动作时"))
			                   .arg(errorInfo.IntLineNum)
			                   .arg(errorInfo.ErrorNum)
			                   .arg(desc);
		} else {
			errorMessage = QString("代码: %1\n描述: %2").arg(errorInfo.ErrorNum).arg(desc);
		}
		QMessageBox dialog(QMessageBox::Critical, tr("Error"), errorMessage, QMessageBox::Ok);
		dialog.exec();
		QSPCallBacks::RefreshInt(QSP_FALSE, QSP_TRUE);
	}
	// request->redirect(QUrl("qsp:/"));
}
