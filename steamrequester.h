#ifndef STEAMWEBAPI_H
#define STEAMWEBAPI_H

#include <QObject>

class QNetworkReply;
class QNetworkAccessManager;

class DatabaseModel;

class SteamRequester : public QObject
{
	Q_OBJECT

public:
	SteamRequester(DatabaseModel *model);
	~SteamRequester();
	bool isRunning() const {return isRunning_;}

public slots:
	void processModName(QNetworkReply *reply = nullptr);
	void requestModNames();

signals:
	void finished();
	void modProcessed();

private:
	void modNameProcessed();

	QNetworkAccessManager *manager_ = nullptr;
	DatabaseModel *model_;
	bool isRunning_ = false;

	int countOfRemainingMods_;
};



#endif // STEAMWEBAPI_H
