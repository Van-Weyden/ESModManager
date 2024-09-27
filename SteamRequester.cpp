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
            QModelIndex modelIndex = m_modDatabaseModel->index(i);
            ModInfo &modInfo = m_modDatabaseModel->modInfoRef(modelIndex);
            if (modInfo.folderName == modFolderName) {
                //During the processing of the request, the data could change, so check again
                if (!steamModName.isEmpty() && !m_modDatabaseModel->isNameValid(modInfo.steamName)) {
                    modInfo.steamName = steamModName;
                    m_modDatabaseModel->updateRow(modelIndex);
                }
                break;
            }
        }
    }
    ///Debug block
//    else if (reply != nullptr) {
//        ModInfo tmp;
//        tmp.name = reply->readAll();
//        tmp.folderName = reply->errorString();
//        tmp.steamName = QString::number(reply->attribute( QNetworkRequest::HttpStatusCodeAttribute).toInt());
//        m_model->appendDatabase(tmp);
//    }

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
        QModelIndex modelIndex = m_modDatabaseModel->index(i);
        ModInfo &modInfo = m_modDatabaseModel->modInfoRef(modelIndex);
        if (!m_modDatabaseModel->isNameValid(modInfo.steamName)) {
            if (ModInfo::isSteamId(modInfo.folderName)) {
                modInfo.steamName = ModInfo::generateWaitingForSteamResponseStub();
                m_modDatabaseModel->updateRow(modelIndex);
                data.clear();
                params.clear();
                params.addQueryItem("itemcount", "1");
                params.addQueryItem("publishedfileids[0]", modInfo.folderName);
                data.append(params.toString().toUtf8());
                request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));

                m_networkAccessManager->post(request, data);
            } else {
                modInfo.steamName = ModInfo::generateFailedToGetNameStub();
                m_modDatabaseModel->updateRow(modelIndex);
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
            QModelIndex modelIndex = m_modDatabaseModel->index(i);
            ModInfo &modInfo = m_modDatabaseModel->modInfoRef(modelIndex);
            if (modInfo.steamName.contains(ModInfo::generateWaitingForSteamResponseStub())) {
                modInfo.steamName = ModInfo::generateFailedToGetNameStub();
                m_modDatabaseModel->updateRow(modelIndex);
            }
        }
        m_isRunning = false;
        emit finished();
    }
}
