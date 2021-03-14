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

#include "DatabaseModel.h"

#include "SteamRequester.h"

//public:

SteamRequester::SteamRequester(DatabaseModel *model)
{
	m_model = model;
	m_manager = new QNetworkAccessManager(this);
	connect(m_manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(processModName(QNetworkReply *)));
}

//public slots:

void SteamRequester::processModName(QNetworkReply *reply)
{
	QString steamModName, modKey;
	QJsonObject modData;
	if (reply != nullptr && reply->error() == QNetworkReply::NoError) {
		modData = QJsonDocument::fromJson(reply->readAll())
					["response"].toObject()
					["publishedfiledetails"].toArray()
					[0].toObject();
		reply->deleteLater();

		modKey = modData["publishedfileid"].toString();
		steamModName = modData["title"].toString();

		for (int i = 0; i < m_model->databaseSize(); ++i) {
			if (m_model->modFolderName(i) == modKey) {
				//During the processing of the request, the data could change, so check again
				if (m_model->modSteamName(i).contains(ModInfo::generateWaitingForSteamResponseStub()) && !steamModName.isEmpty()) {
					m_model->modInfoRef(i).steamName = steamModName;
					emit m_model->dataChanged(m_model->index(i, 0), m_model->index(i, 1));
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
	m_countOfRemainingMods = m_model->databaseSize();
	QByteArray data;
	QUrlQuery params;
	QNetworkRequest request(QUrl("https://api.steampowered.com/ISteamRemoteStorage/GetPublishedFileDetails/v1/"));
	for (int i = 0; i < m_model->databaseSize(); ++i) {
		if (!m_model->isNameValid(m_model->modSteamName(i))) {
			if (ModInfo::isSteamId(m_model->modFolderName(i))) {
				m_model->modInfoRef(i).steamName = ModInfo::generateWaitingForSteamResponseStub();
				emit m_model->dataChanged(m_model->index(i, 0), m_model->index(i, 1));
				data.clear();
				params.clear();
				params.addQueryItem("itemcount", "1");
				params.addQueryItem("publishedfileids[0]", m_model->modFolderName(i));
				data.append(params.toString().toUtf8());
				request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));

				m_manager->post(request, data);
			} else {
				m_model->modInfoRef(i).steamName = ModInfo::generateFailedToGetNameStub();
				emit m_model->dataChanged(m_model->index(i, 0), m_model->index(i, 1));
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
		for (int i = 0; i < m_model->databaseSize(); ++i) {
			if (m_model->modSteamName(i).contains(ModInfo::generateWaitingForSteamResponseStub())) {
				m_model->modInfoRef(i).steamName = ModInfo::generateFailedToGetNameStub();
				emit m_model->dataChanged(m_model->index(i, 0), m_model->index(i, 1));
			}
		}
		m_isRunning = false;
		emit finished();
	}
}
