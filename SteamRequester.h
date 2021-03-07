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
	/**
	 * @brief SteamRequester - Creates an object of SteamRequester with model "model".
	 * SteamRequester does not take ownership of model.
	 */
	SteamRequester(DatabaseModel *model);
	~SteamRequester() = default;
	inline bool isRunning() const;

public slots:
	void processModName(QNetworkReply *reply = nullptr);
	void requestModNames();

signals:
	void finished();
	void modProcessed();

private:
	void modNameProcessed();

	QNetworkAccessManager *m_manager = nullptr;
	DatabaseModel *m_model = nullptr;
	bool m_isRunning = false;

	int m_countOfRemainingMods = 0;
};



//public:

inline bool SteamRequester::isRunning() const
{
	return m_isRunning;
}

#endif // STEAMWEBAPI_H
