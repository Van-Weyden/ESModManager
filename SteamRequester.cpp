#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QHttpPart>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QUrlQuery>

#include "ModDatabaseModel.h"

#include "SteamRequester.h"

//public:

SteamRequester::SteamRequester(ModDatabaseModel *modDatabaseModel)
{
	m_modDatabaseModel = modDatabaseModel;
	m_networkAccessManager = new QNetworkAccessManager(this);
	connect(m_networkAccessManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(processModName(QNetworkReply *)));
}

//public slots:

void SteamRequester::processModName(QNetworkReply *reply)
{
	QString steamModName, modFolderName;
	QJsonObject modData;
	if (reply != nullptr && reply->error() == QNetworkReply::NoError) {
		modData = QJsonDocument::fromJson(reply->readAll())
					["response"].toObject()
					["publishedfiledetails"].toArray()
					[0].toObject();
		reply->deleteLater();

		modFolderName = modData["publishedfileid"].toString();
		steamModName = modData["title"].toString();

		for (int i = 0; i < m_modDatabaseModel->databaseSize(); ++i) {
			if (m_modDatabaseModel->modFolderName(i) == modFolderName) {
				//During the processing of the request, the data could change, so check again
				if (!steamModName.isEmpty() && m_modDatabaseModel->isNameValid(m_modDatabaseModel->modSteamName(i))) {
					m_modDatabaseModel->modInfoRef(i).steamName = steamModName;
					emit m_modDatabaseModel->dataChanged(m_modDatabaseModel->index(i, 0), m_modDatabaseModel->index(i, 1));
				}
				break;
			}
		}
	}
	///Debug block
//	else if (reply != nullptr) {
//		ModInfo tmp;
//		tmp.name = reply->readAll();
//		tmp.folderName = reply->errorString();
//		tmp.steamName = QString::number(reply->attribute( QNetworkRequest::HttpStatusCodeAttribute).toInt());
//		m_model->appendDatabase(tmp);
//	}

	modNameProcessed();
}

void SteamRequester::requestModNames()
{
	if (m_isRunning) {
		return;
	}

	m_isRunning = true;
	m_countOfRemainingMods = m_modDatabaseModel->databaseSize();
	QByteArray data;
	QUrlQuery params;
	QNetworkRequest request(QUrl("https://api.steampowered.com/ISteamRemoteStorage/GetPublishedFileDetails/v1/"));
	for (int i = 0; i < m_modDatabaseModel->databaseSize(); ++i) {
		if (!m_modDatabaseModel->isNameValid(m_modDatabaseModel->modSteamName(i))) {
			if (ModInfo::isSteamId(m_modDatabaseModel->modFolderName(i))) {
				m_modDatabaseModel->modInfoRef(i).steamName = ModInfo::generateWaitingForSteamResponseStub();
				emit m_modDatabaseModel->dataChanged(m_modDatabaseModel->index(i, 0), m_modDatabaseModel->index(i, 1));
				data.clear();
				params.clear();
				params.addQueryItem("itemcount", "1");
				params.addQueryItem("publishedfileids[0]", m_modDatabaseModel->modFolderName(i));
				data.append(params.toString().toUtf8());
				request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));

				m_networkAccessManager->post(request, data);
			} else {
				m_modDatabaseModel->modInfoRef(i).steamName = ModInfo::generateFailedToGetNameStub();
				emit m_modDatabaseModel->dataChanged(m_modDatabaseModel->index(i, 0), m_modDatabaseModel->index(i, 1));
				modNameProcessed();
			}
		} else {
			modNameProcessed();
		}
	}
}

void SteamRequester::modNameProcessed()
{
	emit modProcessed();
	--m_countOfRemainingMods;

	if (!m_countOfRemainingMods) {
		for (int i = 0; i < m_modDatabaseModel->databaseSize(); ++i) {
			if (m_modDatabaseModel->modSteamName(i).contains(ModInfo::generateWaitingForSteamResponseStub())) {
				m_modDatabaseModel->modInfoRef(i).steamName = ModInfo::generateFailedToGetNameStub();
				emit m_modDatabaseModel->dataChanged(m_modDatabaseModel->index(i, 0), m_modDatabaseModel->index(i, 1));
			}
		}
		m_isRunning = false;
		emit finished();
	}
}
