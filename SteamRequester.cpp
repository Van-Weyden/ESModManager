#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QHttpPart>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QThread>
#include <QTimer>
#include <QUrlQuery>

#include "mvc/ModDatabaseModel.h"

#include "SteamRequester.h"

//public:

SteamRequester::SteamRequester(ModDatabaseModel *modDatabaseModel)
{
    m_modDatabaseModel = modDatabaseModel;
    m_networkAccessManager = new QNetworkAccessManager(this);
    connect(m_networkAccessManager, &QNetworkAccessManager::finished, this, &SteamRequester::processModName);
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

        if (QThread::currentThread()->isInterruptionRequested()) {
            onFinished(ModInfo::unknownNameStub());
        } else {
            m_modDatabaseModel->setSteamName(modFolderName, steamModName);
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
    m_countOfRemainingMods = m_modDatabaseModel->size();
    QByteArray data;
    QUrlQuery params;
    QNetworkRequest request(QUrl("https://api.steampowered.com/ISteamRemoteStorage/GetPublishedFileDetails/v1/"));
    for (int i = 0; i < m_modDatabaseModel->size(); ++i) {
        if (QThread::currentThread()->isInterruptionRequested())
        {
            onFinished(ModInfo::unknownNameStub());
            return;
        }

        ModInfo &modInfo = m_modDatabaseModel->modInfoRef(i);
        if (!ModInfo::isNameValid(modInfo.steamName)) {
            if (ModInfo::isSteamId(modInfo.folderName)) {
                modInfo.steamName = ModInfo::waitingForSteamResponseStub();
                m_modDatabaseModel->onModInfoUpdated(i, { Qt::DisplayRole });
                data.clear();
                params.clear();
                params.addQueryItem("itemcount", "1");
                params.addQueryItem("publishedfileids[0]", modInfo.folderName);
                data.append(params.toString().toUtf8());
                request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));

                m_networkAccessManager->post(request, data);
            } else {
                modInfo.steamName = ModInfo::failedToGetNameStub();
                m_modDatabaseModel->onModInfoUpdated(i, { Qt::DisplayRole });
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
        onFinished(ModInfo::failedToGetNameStub());
    }
}

void SteamRequester::onFinished(const QString &failedStub)
{
    for (int i = 0; i < m_modDatabaseModel->size(); ++i) {
        ModInfo &modInfo = m_modDatabaseModel->modInfoRef(i);
        if (modInfo.steamName == ModInfo::waitingForSteamResponseStub()) {
            modInfo.steamName = failedStub;
            m_modDatabaseModel->onModInfoUpdated(i, { Qt::DisplayRole });
        }
    }
    m_isRunning = false;
    emit finished();
}
