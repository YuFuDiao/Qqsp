#ifndef QQSP_DEBUGGER_DEBUGWINDOW_H
#define QQSP_DEBUGGER_DEBUGWINDOW_H

#include <QMainWindow>

class QItemSelectionModel;
class QSettings;

namespace Ui {
	class DebugWindow;
}

namespace Debugger {
	class ExpressionsModel;

	class DebugWindow: public QMainWindow {
		using base = QMainWindow;
		Q_OBJECT

	public:
		DebugWindow(QAction* toggleVisibilityAction, QWidget* parent = nullptr);
		~DebugWindow();

		void appendLogLine(QString line);
		void scheduleUpdate();

		void saveSettings(QSettings& settings);
		void loadSettings(QSettings& settings);

		// QWidget interface
		void setVisible(bool visible) override;

	private:
		void copyLogToClipboard();
		void refreshAllExpressions();
		void runCode();
		void expressionsSelectionChanged(const QModelIndex& current, const QModelIndex& previous);
		void expressionsRowsInserted(const QModelIndex& parent, int first, int last);
		void expressionsRowsRemoved(const QModelIndex& parent, int first, int last);
		void expressionsModelReset();
		void addNewExpression();
		void removeSelectedExpression();
		void removeAllExpressions();
		void copySelectedExpressionValue();
		void copyAllExpression();

		Ui::DebugWindow* ui_;
		QAction* toggleVisibilityAction_;
		ExpressionsModel* expressionsModel_;
		QItemSelectionModel* expressionSelectionModel_;
		bool updateScheduled_{};
	};
} // namespace Debugger
#endif
