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

#include "databasemodel.h"

#include "steamrequester.h"

SteamRequester::SteamRequester(DatabaseModel *model)
{
	model_ = model;
	manager_ = new QNetworkAccessManager(this);
	connect(manager_, SIGNAL(finished(QNetworkReply *)), this, SLOT(processModName(QNetworkReply *)));
}

 SteamRequester::~SteamRequester()
{
	 delete manager_;
}

//public slots:

void SteamRequester::processModName(QNetworkReply *reply)
{
	QString steamModName, modKey;
	QJsonObject modData;
	if (reply != nullptr && reply->error() == QNetworkReply::NoError) {
		modData = QJsonDocument::fromJson(reply->readAll())
					["response"].toObject()
					["publishedfiledetails"].toArray()[0].toObject();
		reply->deleteLater();

		modKey = modData["publishedfileid"].toString();
		steamModName = modData["title"].toString();

		for (int i = 0; i < model_->databaseSize(); ++i) {
			if (model_->modFolderName(i) == modKey) {
				//During the processing of the request, the data could change, so check again
				if (model_->modSteamName(i).contains(tr("Waiting for Steam mod name response..."))) {
					model_->modInfoRef(i).steamName = steamModName;
					emit model_->dataChanged(model_->index(i, 0), model_->index(i, 1));
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
//		model_->appendDatabase(tmp);
//	}


	modNameProcessed();
}

void SteamRequester::requestModNames()
{
	if (isRunning_) {
		return;
	}

	isRunning_ = true;
	countOfRemainingMods_ = model_->databaseSize();
	QByteArray data;
	QUrlQuery params;
	QNetworkRequest request(QUrl("https://api.steampowered.com/ISteamRemoteStorage/GetPublishedFileDetails/v1/"));
	for (int i = 0; i < model_->databaseSize(); ++i) {
		if (!model_->isValidName(model_->modSteamName(i))) {
			model_->modInfoRef(i).steamName = tr("Waiting for Steam mod name response...");
			emit model_->dataChanged(model_->index(i, 0), model_->index(i, 1));
			if (ModInfo::isSteamId(model_->modFolderName(i))) {
				data.clear();
				params.clear();
				params.addQueryItem("itemcount", "1");
				params.addQueryItem("publishedfileids[0]", model_->modFolderName(i));
				data.append(params.toString());
				request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));

				manager_->post(request, data);
			}
			else {
				processModName();
			}
		}
		else {
			processModName();
		}
	}
}

void SteamRequester::modNameProcessed()
{
	emit modProcessed();
	--countOfRemainingMods_;
	if (!countOfRemainingMods_) {
		for (int i = 0; i < model_->databaseSize(); ++i)
			if (model_->modSteamName(i).contains(tr("Waiting for Steam mod name response..."))) {
				model_->modInfoRef(i).steamName = tr("WARNING: couldn't get the name of mod. Set the name manually.");
				emit model_->dataChanged(model_->index(i, 0), model_->index(i, 1));
			}
		isRunning_ = false;
		emit finished();
	}

}
