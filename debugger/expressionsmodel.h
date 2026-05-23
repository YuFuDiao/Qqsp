#ifndef QQSP_DEBUGGER_DEBUGEXPRESSIONSMODEL_H
#define QQSP_DEBUGGER_DEBUGEXPRESSIONSMODEL_H

#include <QAbstractTableModel>
#include <QVector>

class QSettings;

namespace Debugger {
	class ExpressionsModel: public QAbstractTableModel {
		using Base = QAbstractTableModel;
		Q_OBJECT

	public:
		explicit ExpressionsModel(QObject* parent = nullptr);

		QModelIndex addExpression(const QString& expression);
		bool evaluate(qsizetype index = -1);
		bool evaluateAll();
		const QString& expression(int row) const;
		const QString& value(int row) const;
		void clear();

		void saveSettings(QSettings& settings) const;
		void loadSettings(const QSettings& settings);

		// QAbstractItemModel interface
		int rowCount(const QModelIndex& parent = {}) const override;
		int columnCount(const QModelIndex& parent) const override;
		Qt::ItemFlags flags(const QModelIndex& index) const override;
		QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
		QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
		bool setData(const QModelIndex& index, const QVariant& value, int role) override;
		bool insertRows(int row, int count, const QModelIndex& parent) override;
		bool removeRows(int row, int count, const QModelIndex& parent) override;

	private:
		struct Expression {
			Expression(QString exp);

			QString expression;
			QString value;
			bool changed;
		};

		QVector<Expression> entries_;
	};
} // namespace Debugger
#endif
