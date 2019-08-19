#include <QCollator>
#include <QColor>
#include <QThread>

#include "steamrequester.h"

#include "databasemodel.h"

//public:
DatabaseModel::DatabaseModel()
{
	qRegisterMetaType<QVector<int> >("QVector<int>");
}

DatabaseModel::~DatabaseModel()
{
}

QVariant DatabaseModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	switch (role) {
		case Qt::DisplayRole:
			if (completeModNames_) {
				if (useSteamModNames_) {
					if (isValidName(database_.at(index.row()).steamName) ||
						!isValidName(database_.at(index.row()).name))
						return database_.at(index.row()).steamName;
					else
						return database_.at(index.row()).name;
				}
				else {
					if (isValidName(database_.at(index.row()).name) ||
						!isValidName(database_.at(index.row()).steamName))
						return database_.at(index.row()).name;
					else
						return database_.at(index.row()).steamName;
				}
			}
			else {
				if (useSteamModNames_)
					return database_.at(index.row()).steamName;
				else
					return database_.at(index.row()).name;
			}

		case Qt::ForegroundRole:
			if (!database_.at(index.row()).exists)
				return QColor(Qt::gray);
			break;

		case Qt::CheckStateRole:
			if (index.column() == 0) {
				if (database_.at(index.row()).enabled)
					return Qt::Checked;
				else
					return Qt::Unchecked;
			}
	}

	return QVariant();
}

Qt::ItemFlags DatabaseModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = QAbstractItemModel::flags(index);
	if (index.column() == 0)
		flags |= Qt::ItemIsUserCheckable;
	return flags;
}

QVariant DatabaseModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal)
			return tr("Mod name");
		else
			return QString::number(section + 1);
	}
	return QVariant();
}

void DatabaseModel::removeFromDatabase(const int index)
{
	beginRemoveRows(QModelIndex(), index, index);

	database_.removeAt(index);

	endRemoveRows();
}

void DatabaseModel::setCompleteModNames(const bool &enabled)
{
	if (completeModNames_ == enabled)
		return;

	completeModNames_ = enabled;
	emit dataChanged(index(0), index(database_.size() - 1));
}

bool DatabaseModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid())
		return false;

	bool isDataChanged = false;

	switch (role) {
		case Qt::DisplayRole:
			if (useSteamModNames_)
				database_[index.row()].steamName = value.toString();
			else
				database_[index.row()].name = value.toString();
			isDataChanged = true;
		break;

		case Qt::CheckStateRole:
			if (index.column() == 0) {
				database_[index.row()].enabled = (Qt::CheckState(value.toInt()) == Qt::Checked);
				emit modCheckStateChanged(index.row(), database_.at(index.row()).enabled);
				isDataChanged = true;
			}
		break;
	}

	if (isDataChanged)
		emit dataChanged(index, index, QVector<int>(role));

	return isDataChanged;
}

void DatabaseModel::setModsExistsState(const bool &isExists)
{
	for (int i = 0; i < database_.size(); ++i)
		database_[i].exists = isExists;
}

void DatabaseModel::setUsingSteamModNames(const bool &use)
{
	if (useSteamModNames_ == use)
		return;

	useSteamModNames_ = use;
	emit dataChanged(index(0), index(database_.size() - 1));
}

void DatabaseModel::sortDatabase()
{
	//Сортировка имён, чтобы порядок сответвовал тому, что в проводнике
	QCollator collator;
	collator.setNumericMode(true);
	std::sort(database_.begin(), database_.end(),
			[&collator](const ModInfo &i, const ModInfo &j)
			{
				if (i.exists ^ j.exists)
					return i.exists;
				return collator(i.folderName, j.folderName);
			});
}

//public slots:

void DatabaseModel::enableMod(const QModelIndex &index)
{
	if (!database_.at(index.row()).enabled) {
		database_[index.row()].enabled = true;
		emit dataChanged(index, index);
		emit modCheckStateChanged(index.row(), true);
	}
}

void DatabaseModel::disableMod(const QModelIndex &index)
{
	if (database_.at(index.row()).enabled) {
		database_[index.row()].enabled = false;
		emit dataChanged(index, index);
		emit modCheckStateChanged(index.row(), false);
	}
}

//protected slots:
void DatabaseModel::setCompleteModNames(const int &mode)
{
	if (mode == Qt::CheckState::Checked)
		setCompleteModNames(true);
	else if (mode == Qt::CheckState::Unchecked)
		setCompleteModNames(false);
}

void DatabaseModel::setUsingSteamModNames(const int &mode)
{
	if (mode == Qt::CheckState::Checked)
		setUsingSteamModNames(true);
	else if (mode == Qt::CheckState::Unchecked)
		setUsingSteamModNames(false);
}
