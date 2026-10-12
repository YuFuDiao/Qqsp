#include "callbacks_gui.h"

#include "comtools.h"
#include "debugger/debugwindow.h"
#include "qspmsgdlg.h"

#include <QAudioOutput>
#include <QByteArray>
#include <QCoreApplication>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QThread>
#include <QTimer>

#ifdef QT_WEBENGINEWIDGETS_LIB
#	include "qspwebbox.h"
#endif

QString QSPCallBacks::m_gamePath;
MainWindow* QSPCallBacks::m_frame;
Debugger::DebugWindow* QSPCallBacks::m_debugWindow;
bool QSPCallBacks::m_isHtml;
QSPSounds QSPCallBacks::m_sounds;
float QSPCallBacks::m_volumeCoeff;
bool QSPCallBacks::m_isAllowHTML5Extras;

namespace {
	QByteArray loadFile(const QString& path)
	{
		if (path.isEmpty()) {
			return {};
		}

		QFileInfo fileInfo(path);
		if (fileInfo.exists() && fileInfo.isFile()) {
			QFile inp{fileInfo.filePath()};
			if (inp.open(QIODevice::ReadOnly)) {
				return inp.readAll();
			}
		}
		return {};
	}

	QByteArray loadFile(const QSPString& path)
	{
		if (!path.Str) {
			return {};
		}

		return loadFile(QSPTools::qspStrToQt(path));
	}
} // namespace

void QSPCallBacks::Init(MainWindow* frame, Debugger::DebugWindow* debugWindow)
{
	m_frame = frame;
	m_debugWindow = debugWindow;
	m_volumeCoeff = 1.0f;

	m_isAllowHTML5Extras = false;

	/* void func(QSPString str) */
	QSPSetCallback(QSP_CALL_DEBUG, (QSP_CALLBACK)&Debug);
	// QSP_CALL_ISPLAYINGFILE, /* QSP_BOOL func(QSPString file) */
	QSPSetCallback(QSP_CALL_ISPLAYINGFILE, (QSP_CALLBACK)&IsPlay);
	// QSP_CALL_PLAYFILE,            /* void func(QSPString file, int volume) */
	QSPSetCallback(QSP_CALL_PLAYFILE, (QSP_CALLBACK)&PlayFile);
	// QSP_CALL_CLOSEFILE,       /* void func(QSPString file) */
	QSPSetCallback(QSP_CALL_CLOSEFILE, (QSP_CALLBACK)&CloseFile);
	// QSP_CALL_SHOWIMAGE,       /* void func(QSPString file) */
	QSPSetCallback(QSP_CALL_SHOWIMAGE, (QSP_CALLBACK)&ShowImage);
	// QSP_CALL_SHOWWINDOW,      /* void func(int type, QSP_BOOL toShow) */
	QSPSetCallback(QSP_CALL_SHOWWINDOW, (QSP_CALLBACK)&ShowPane);
	// QSP_CALL_SHOWMENU,        /* int func(QSPListItem *items, int count) */
	QSPSetCallback(QSP_CALL_SHOWMENU, (QSP_CALLBACK)&ShowMenu);
	// QSP_CALL_SHOWMSGSTR,      /* void func(QSPString text) */
	QSPSetCallback(QSP_CALL_SHOWMSGSTR, (QSP_CALLBACK)&Msg);
	// QSP_CALL_REFRESHINT,      /* void func(QSP_BOOL isForced) */
	QSPSetCallback(QSP_CALL_REFRESHINT, (QSP_CALLBACK)&RefreshInt);
	// QSP_CALL_SETTIMER,        /* void func(int msecs) */
	QSPSetCallback(QSP_CALL_SETTIMER, (QSP_CALLBACK)&SetTimer);
	// QSP_CALL_SETINPUTSTRTEXT, /* void func(QSPString text) */
	QSPSetCallback(QSP_CALL_SETINPUTSTRTEXT, (QSP_CALLBACK)&SetInputStrText);
	// QSP_CALL_SYSTEM,          /* void func(QSPString cmd) */
	// QSP_CALL_OPENGAME,        /* void func(QSP_BOOL isNewGame) */
	QSPSetCallback(QSP_CALL_OPENGAME, (QSP_CALLBACK)&OpenGame);
	// QSP_CALL_OPENGAMESTATUS,  /* void func(QSPString file) */
	QSPSetCallback(QSP_CALL_OPENGAMESTATUS, (QSP_CALLBACK)&OpenGameStatus);
	// QSP_CALL_SAVEGAMESTATUS,  /* void func(QSPString file) */
	QSPSetCallback(QSP_CALL_SAVEGAMESTATUS, (QSP_CALLBACK)&SaveGameStatus);
	// QSP_CALL_SLEEP,           /* void func(int msecs) */
	QSPSetCallback(QSP_CALL_SLEEP, (QSP_CALLBACK)&Sleep);
	// QSP_CALL_GETMSCOUNT,      /* int func() */
	QSPSetCallback(QSP_CALL_GETMSCOUNT, (QSP_CALLBACK)&GetMSCount);
	// QSP_CALL_INPUTBOX, /* void func(QSPString text, QSP_CHAR *buffer, int
	// maxLen) */
	QSPSetCallback(QSP_CALL_INPUTBOX, (QSP_CALLBACK)&Input);
	// QSP_CALL_VERSION,  /* void func(QSPString param, QSP_CHAR *buffer, int
	// maxLen) */
}

void QSPCallBacks::DeInit()
{
	CloseFile({nullptr, nullptr});
}

void QSPCallBacks::SetTimer(int msecs)
{
	if (m_frame->IsQuit()) {
		return;
	}
	if (msecs) {
		m_frame->GetTimer()->start(msecs);
	} else {
		m_frame->GetTimer()->stop();
	}
}

void QSPCallBacks::RefreshInt(QSP_BOOL isForced, QSP_BOOL isNewDesc)
{
	using namespace Qt::StringLiterals;

	QSPVariant val;
	bool isScroll, isCanSave;
	if (m_frame->IsQuit()) {
		return;
	}
	// -------------------------------
	UpdateGamePath(m_frame->gameFilePath());
	// -------------------------------
	const QSPString mainDesc = QSPGetMainDesc();
	const QSPString varsDesc = QSPGetVarsDesc();
	// -------------------------------
	isScroll = !(QSPGetVarValue(L"DISABLESCROLL"_qsp, 0, &val) && QSP_NUM(val));
	isCanSave = !(QSPGetVarValue(L"NOSAVE"_qsp, 0, &val) && QSP_NUM(val));
	m_isHtml = QSPGetVarValue(L"USEHTML"_qsp, 0, &val) && QSP_NUM(val);
	// -------------------------------
	m_frame->GetVars()->SetIsHtml(m_isHtml);
	const int windowStateChanges = QSPGetWindowsChangedState();
	if (windowStateChanges & QSP_WIN_VARS) {
		m_frame->EnableControls(false, true);
		if (m_isAllowHTML5Extras) {
			if (QSPGetVarValue(L"SETSTATHEAD"_qsp, 0, &val) && QSP_STR(val).Str) {
				m_frame->GetVars()->SetHead(QSPTools::qspStrToQt(QSP_STR(val)));
			} else {
				m_frame->GetVars()->SetHead(QString(""));
			}
		}
		m_frame->GetVars()->SetText(QSPTools::qspStrToQt(varsDesc), isScroll);
		m_frame->EnableControls(true, true);
	}
	// -------------------------------
	isScroll = !isNewDesc;
	m_frame->GetDesc()->SetIsHtml(m_isHtml);
	if (windowStateChanges & QSP_WIN_MAIN) {
		m_frame->EnableControls(false, true);
		if (m_isAllowHTML5Extras) {
			if (QSPGetVarValue(L"SETMAINDESCHEAD"_qsp, 0, &val) && QSP_STR(val).Str) {
				m_frame->GetDesc()->SetHead(QSPTools::qspStrToQt(QSP_STR(val)));
			} else {
				m_frame->GetDesc()->SetHead(QString(""));
			}
		}
		m_frame->GetDesc()->SetText(QSPTools::qspStrToQt(mainDesc), isScroll);
		m_frame->EnableControls(true, true);
	}
	// -------------------------------
	m_frame->GetActions()->SetIsHtml(m_isHtml);
	m_frame->GetActions()->SetIsShowNums(m_frame->IsShowHotkeys());
	if (windowStateChanges & QSP_WIN_ACTS) {
		std::vector<QSPListItem> actions = QSPTools::qspActions();
		m_frame->GetActions()->BeginItems();
		for (const QSPListItem& li: actions) {
			m_frame->GetActions()->AddItem(QSPTools::GetCaseInsensitiveFilePath(m_gamePath, QSPTools::qspStrToQt(li.Image)), QSPTools::qspStrToQt(li.Name));
		}
		m_frame->GetActions()->EndItems();
	}
	m_frame->GetActions()->SetSelection(QSPGetSelActionIndex());
	m_frame->GetObjects()->SetIsHtml(m_isHtml);
	if (windowStateChanges & QSP_WIN_OBJS) {
		std::vector<QSPObjectItem> objects = QSPTools::qspObjects();
		m_frame->GetObjects()->BeginItems();
		for (const QSPObjectItem& li: objects) {
			m_frame->GetObjects()->AddItem(
				QSPTools::GetCaseInsensitiveFilePath(m_gamePath, QSPTools::qspStrToQt(li.Image)),
				"%1"_L1.arg(QSPTools::qspStrToQt(li.Name)));
		}
		m_frame->GetObjects()->EndItems();
	}
	m_frame->GetObjects()->SetSelection(QSPGetSelObjectIndex());
	// -------------------------------
	if (QSPGetVarValue(L"BACKIMAGE"_qsp, 0, &val) && QSP_STR(val).Str) {
		m_frame->GetDesc()->LoadBackImage(QSPTools::GetCaseInsensitiveFilePath(m_gamePath, QSPTools::qspStrToQt(QSP_STR(val))));
	} else {
		m_frame->GetDesc()->LoadBackImage(QString(""));
	}
	// -------------------------------
	m_frame->ApplyParams();
	if (isForced) {
		m_frame->EnableControls(false, true);
		// m_frame->Update();
		// QCoreApplication::processEvents();
		if (m_frame->IsQuit()) {
			return;
		}
		m_frame->EnableControls(true, true);
	}
	m_frame->GetGameMenu()->setEnabled(isCanSave);
}

void QSPCallBacks::SetInputStrText(QSPString text)
{
	if (m_frame->IsQuit()) {
		return;
	}
	m_frame->GetInput()->SetText(QSPTools::qspStrToQt(text));
}

QSP_BOOL QSPCallBacks::IsPlay(QSPString file)
{
	QSP_BOOL playing = QSP_FALSE;
	QSPSounds::iterator elem =
		m_sounds.find(QFileInfo(m_gamePath + QSPTools::GetCaseInsensitiveFilePath(m_gamePath, QSPTools::qspStrToQt(file))).absoluteFilePath());
	if (elem != m_sounds.end()) {
		if (elem->second.player->isPlaying()) {
			playing = QSP_TRUE;
		}
	}
	return playing;
}

void QSPCallBacks::CloseFile(QSPString file)
{
	if (file.Str) {
		QSPSounds::iterator elem =
			m_sounds.find(QFileInfo(m_gamePath + QSPTools::GetCaseInsensitiveFilePath(m_gamePath, QSPTools::qspStrToQt(file))).absoluteFilePath());
		if (elem != m_sounds.end()) {
			m_sounds.erase(elem);
		}
	} else {
		for (QSPSounds::iterator i = m_sounds.begin(); i != m_sounds.end(); ++i) {
			m_sounds.clear();
		}
	}
}

void QSPCallBacks::PlayFile(QSPString file, int volume)
{
	if (SetVolume(file, volume)) {
		return;
	}
	CloseFile(file);
	QString strFile(QFileInfo(m_gamePath + QSPTools::GetCaseInsensitiveFilePath(m_gamePath, QSPTools::qspStrToQt(file))).absoluteFilePath());
	QSPSound& sound = m_sounds.emplace(strFile, QSPSound{}).first->second;
	sound.player->setSource(QUrl::fromLocalFile(strFile));
	sound.output->setVolume(volume * m_volumeCoeff);
	sound.player->play();
	UpdateSounds();
}

void QSPCallBacks::ShowPane(int type, QSP_BOOL isShow)
{
	if (m_frame->IsQuit()) {
		return;
	}
	if (type & QSP_WIN_ACTS) {
		m_frame->GetActionsDock()->setVisible(isShow != QSP_FALSE);
	}
	if (type & QSP_WIN_OBJS) {
		m_frame->GetObjectsDock()->setVisible(isShow != QSP_FALSE);
	}
	if (type & QSP_WIN_VARS) {
		m_frame->GetVarsDock()->setVisible(isShow != QSP_FALSE);
	}
	if (type & QSP_WIN_INPUT) {
		m_frame->GetInputDock()->setVisible(isShow != QSP_FALSE);
	}
	if (type & QSP_WIN_VIEW) {
		m_frame->GetImageDock()->setVisible(isShow != QSP_FALSE);
	}
}

void QSPCallBacks::Sleep(int msecs)
{
	QTimer wtimer;
	wtimer.setSingleShot(true);
	QEventLoop loop;
	QObject::connect(&wtimer, &QTimer::timeout, &loop, &QEventLoop::quit);
	wtimer.start(50);
	loop.exec();
	// RefreshInt(QSP_TRUE);
	if (m_frame->IsQuit()) {
		return;
	}
	bool isSave = m_frame->GetGameMenu()->isEnabled();
	bool isBreak = false;
	m_frame->EnableControls(false, true);
	int i, count = msecs / 50;
	for (i = 0; i < count; ++i) {
		// QThread::msleep(50);
		wtimer.start(50);
		loop.exec();
		// qDebug() << QSPTools::qspStrToQt(QSPGetMainDesc());
		// m_frame->Update();
		// QCoreApplication::processEvents();
		if (m_frame->IsQuit() || m_frame->IsKeyPressedWhileDisabled()) // TODO: implement
		{
			isBreak = true;
			break;
		}
	}
	if (!isBreak) // NOTE: no check in old code
	{
		// QThread::msleep(msecs % 50);
		wtimer.start(msecs % 50);
		loop.exec();
		// m_frame->Update();
		// QCoreApplication::processEvents();
	}
	m_frame->EnableControls(true, true);
	m_frame->GetGameMenu()->setEnabled(isSave);
}

int QSPCallBacks::GetMSCount()
{
	static QElapsedTimer stopWatch;
	if (stopWatch.isValid() == false) {
		stopWatch.start();
	}
	int ret = stopWatch.restart();
	return ret;
}

void QSPCallBacks::Msg(QSPString str)
{
	if (m_frame->IsQuit()) {
		return;
	}
	RefreshInt(QSP_FALSE, QSP_FALSE);
	QspMsgDlg dialog(
		m_frame->GetDesc()->GetBackgroundColor(),
		m_frame->GetDesc()->GetForegroundColor(),
		m_frame->GetDesc()->GetTextFont(),
		MainWindow::tr("Info"), // caption
		QSPTools::qspStrToQt(str),
		m_isHtml,
		m_gamePath,
		m_frame);
	m_frame->EnableControls(false);
	dialog.exec();
	m_frame->EnableControls(true);
}

void QSPCallBacks::DeleteMenu()
{
	if (m_frame->IsQuit()) {
		return;
	}
	m_frame->DeleteMenu();
}

void QSPCallBacks::AddMenuItem(const QSPString& name, const QSPString& imgPath)
{
	if (m_frame->IsQuit()) {
		return;
	}
	m_frame->AddMenuItem(QSPTools::qspStrToQt(name), QSPTools::GetCaseInsensitiveFilePath(m_gamePath, QSPTools::qspStrToQt(imgPath)));
}

int QSPCallBacks::ShowMenu(const QSPListItem* items, int count)
{
	if (m_frame->IsQuit()) {
		return -1;
	}
	m_frame->EnableControls(false);
	DeleteMenu();
	if (!count) {
		return 0;
	}
	for (int i = 0; i < count; ++i) {
		AddMenuItem(items[i].Name, items[i].Image);
	}
	int index = m_frame->ShowMenu();
	m_frame->EnableControls(true);
	return index;
}

void QSPCallBacks::Input(QSPString text, QSP_CHAR* buffer, int maxLen)
{
	if (m_frame->IsQuit()) {
		return;
	}
	RefreshInt(QSP_FALSE, QSP_FALSE);
	//	QSPInputDlg dialog(m_frame,
	//		wxID_ANY,
	//		m_frame->GetDesc()->GetBackgroundColor(),
	//		m_frame->GetDesc()->GetForegroundColor(),
	//		m_frame->GetDesc()->GetTextFont(),
	//		_("Input data"),
	//		wxString(text.Str, text.End),
	//		m_isHtml,
	//		m_gamePath
	//	);
	//	m_frame->EnableControls(false);
	//	dialog.ShowModal();
	//	m_frame->EnableControls(true);
	// #ifdef _UNICODE
	// 	wcsncpy(buffer, inputText.data(), maxLen);
	// #else
	// 	strncpy(buffer, dialog.GetText().c_str(), maxLen);
	// #endif
	// QString inputText = QInputDialog::getMultiLineText(m_frame,
	// MainWindow::tr("Input data"), QSPTools::qspStrToQt(text));

	QString inputText = QInputDialog::getText(m_frame, MainWindow::tr("Input data"), QSPTools::qspStrToQt(text), QLineEdit::Normal);
	QSPTools::qtStrToQspBuffer(inputText, buffer, maxLen);
}

void QSPCallBacks::ShowImage(QSPString file)
{
	if (m_frame->IsQuit()) {
		return;
	}
	m_frame->GetImgView()->OpenFile(
		QSPTools::GetCaseInsensitiveFilePath(
			m_gamePath,
			QSPTools::qspStrToQt(file))); // NOTE: will not display image if file is not found
	if (QSPTools::qspStrToQt(file) == "") {
		m_frame->GetImageDock()->setVisible(false);
	} else {
		m_frame->GetImageDock()->setVisible(true);
	}

	// m_frame->GetImgView()->setVisible(true);
}

void QSPCallBacks::OpenGame(QSPString file, QSP_BOOL isNewGame)
{
	if (m_frame->IsQuit()) {
		return;
	}
	if (!file.Str) {
		return;
	}
	QByteArray fileData{loadFile(file)};
	if (QSPLoadGameWorldFromData(fileData.data(), fileData.size(), isNewGame) && isNewGame) {
		QFileInfo fileName(QSPTools::qspStrToQt(file));
		m_gamePath = fileName.absolutePath();
		if (!m_gamePath.endsWith(QLatin1Char('/'))) {
			m_gamePath += QLatin1Char('/');
		}
		m_frame->UpdateGamePath(m_gamePath);
	}
	m_debugWindow->scheduleUpdate();
}

bool QSPCallBacks::OpenGameStatusEx(const QSPString& file, bool isReferesh)
{
	if (m_frame->IsQuit()) {
		return false;
	}

	const auto openSave = [isReferesh](QString path) -> bool {
		QByteArray data = loadFile(path);
		if (!data.isEmpty()) {
			return QSPOpenSavedGameFromData(data.data(), data.size(), isReferesh ? QSP_TRUE : QSP_FALSE);
		}
		return false;
	};

	if (file.Str) {
		return openSave(QSPTools::qspStrToQt(file));
	} else {
		m_frame->EnableControls(false);
		QString path =
			QFileDialog::getOpenFileName(m_frame, MainWindow::tr("Select saved game file"), m_frame->GetLastPath(), MainWindow::tr("Saved game files (*.sav)"));
		m_frame->EnableControls(true);
		if (!path.isEmpty()) {
			m_frame->SetLastPath(QFileInfo(path).absolutePath());
			return openSave(path);
		}
		return false;
	}
}

void QSPCallBacks::OpenGameStatus(QSPString file)
{
	OpenGameStatusEx(file, false);
}

bool QSPCallBacks::SaveGameStatusEx(const QSPString& file, bool isRefresh)
{
	if (m_frame->IsQuit()) {
		return false;
	}

	const auto makeSaveFile = [isRefresh](QString file) -> bool {
		int requiredBufSize = 0;
		QSPSaveGameAsData(nullptr, &requiredBufSize, isRefresh ? QSP_TRUE : QSP_FALSE);
		QByteArray buf{requiredBufSize, Qt::Uninitialized};
		if (QSPSaveGameAsData(buf.data(), &requiredBufSize, isRefresh ? QSP_TRUE : QSP_FALSE)) {
			QFile out{file};
			if (!out.open(QIODevice::WriteOnly)) {
				return false;
			}
			return out.write(buf) == buf.size();
		}
		return false;
	};
	if (file.Str) {
		return makeSaveFile(QSPTools::qspStrToQt(file));
	} else {
		m_frame->EnableControls(false);
		QString path =
			QFileDialog::getSaveFileName(m_frame, MainWindow::tr("Select file to save"), m_frame->GetLastPath(), MainWindow::tr("Saved game files (*.sav)"));
		m_frame->EnableControls(true);
		if (!path.isEmpty()) {
			m_frame->SetLastPath(QFileInfo(path).absolutePath());
			return makeSaveFile(path);
		}
		return false;
	}
}

void QSPCallBacks::Debug(QSPString str)
{
	m_debugWindow->appendLogLine(QSPTools::qspStrToQt(str));
}

void QSPCallBacks::SaveGameStatus(QSPString file)
{
	SaveGameStatusEx(file, false);
}

void QSPCallBacks::UpdateGamePath(const QString& fileName)
{
	QFileInfo fileInfo(fileName);
	m_gamePath = fileInfo.absolutePath();
	if (!m_gamePath.endsWith(QLatin1Char('/'))) {
		m_gamePath += QLatin1Char('/');
	}
	// m_frame->UpdateGamePath(m_gamePath);
	m_frame->GetDesc()->SetGamePath(m_gamePath);
	m_frame->GetObjects()->SetGamePath(m_gamePath);
	m_frame->GetActions()->SetGamePath(m_gamePath);
	m_frame->GetVars()->SetGamePath(m_gamePath);
	m_frame->GetImgView()->SetGamePath(m_gamePath);
}

bool QSPCallBacks::SetVolume(const QSPString& file, int volume)
{
	if (!IsPlay(file)) {
		return false;
	}
	QSPSounds::iterator elem =
		m_sounds.find(QString(QFileInfo(m_gamePath + QSPTools::GetCaseInsensitiveFilePath(m_gamePath, QSPTools::qspStrToQt(file))).absoluteFilePath()));
	elem->second.output->setVolume(volume * m_volumeCoeff);
	return true;
}

void QSPCallBacks::SetOverallVolume(float coeff)
{
	m_volumeCoeff = std::clamp(coeff, 0.f, 1.f);
	for (auto& [_, sound]: m_sounds) {
		if (sound.player->isPlaying()) {
			sound.player->audioOutput()->setVolume(sound.player->audioOutput()->volume() * m_volumeCoeff);
		}
	}
}

void QSPCallBacks::SetAllowHTML5Extras(bool HTML5Extras)
{
	m_isAllowHTML5Extras = HTML5Extras;
}

void QSPCallBacks::UpdateSounds()
{
	QSPSounds::iterator i = m_sounds.begin();
	while (i != m_sounds.end()) {
		if (i->second.player->isPlaying()) {
			++i;
		} else {
			i = m_sounds.erase(i);
		}
	}
}

QSPSound::QSPSound()
	: player{new QMediaPlayer()}
	, output{new QAudioOutput()}
{
	player->setAudioOutput(output);
}

QSPSound::QSPSound(QSPSound&& other)
	: player{std::exchange(other.player, nullptr)}
	, output{std::exchange(other.output, nullptr)}
{
}

QSPSound::~QSPSound()
{
	delete output;
	delete player;
}
