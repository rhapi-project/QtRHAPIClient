#include "rhapiclient.h"

#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>

RHAPIClient::RHAPIClient(const QString& authBaseUrl, const QByteArray& appToken, QObject *parent)
    : QObject(parent), authBaseUrl(authBaseUrl), appToken(appToken)
{

}

bool RHAPIClient::auth(const QString& user, const QString& password, QVariantMap& error)
{
    this->user = user;
    this->password = password;
    return auth(error);
}

bool RHAPIClient::auth(QVariantMap& error)
{
    QNetworkRequest authRequest;
    authRequest.setUrl(QUrl(authBaseUrl + "?user=" + this->user + "&password=" + this->password));
    authRequest.setRawHeader("Authorization", QByteArray("Bearer ") + appToken);

    QEventLoop loop;
    bool ok = true;

    QNetworkReply* reply = manager.get(authRequest);

    QObject::connect(reply,
                     static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                     this, [this, &loop, &ok, reply, &error](QNetworkReply::NetworkError code) {
        qWarning("Erreur requête %s %d : %s", qPrintable(reply->url().toString()), code, qPrintable(reply->errorString()));
        QString message;
        if (reply->isReadable()) {
            error = QJsonDocument::fromJson(reply->readAll()).toVariant().toMap();
            //qWarning() << doc.object();
            message = error.value("userMessage").toString();
            if (message.isEmpty()) {
                message = reply->errorString();
            }
        }
        else {
            message = "Déconnexion réseau.";
        }

        error.insert("userMessage", message);

        if (code == QNetworkReply::NetworkSessionFailedError ||
                code == QNetworkReply::HostNotFoundError ||
                code == QNetworkReply::UnknownNetworkError) {
        };

        if (code != QNetworkReply::NoError) {
            qWarning() << message;
        }

        ok = false;
    });

    connect(reply, &QNetworkReply::finished,
            [this, &loop, &ok, reply](){
        //
        if (!reply->isReadable()) {
            ok = false;
            loop.exit();
            return;
        }
        QVariantMap result = QJsonDocument::fromJson(reply->readAll()).toVariant().toMap();
        apiBaseUrl = result.value("url").toString();
        if (apiBaseUrl.isEmpty()) {
            qWarning() << result;
            ok = false;
            loop.exit();
            return;
        }
        apiToken = result.value("token").toByteArray();
        loop.exit();
    });

    loop.exec();
    reply->blockSignals(true);
    reply->deleteLater();
    authorized = ok;
    return ok;
}

bool RHAPIClient::sendRequest(const QString& path, const QString& query, const QString& method, QVariantMap &retDatas)
{
    if (!authorized && !auth(retDatas)) {
        return false;
    }

    QNetworkRequest apiRequest;
    apiRequest.setRawHeader("Authorization", QByteArray("Bearer ") + apiToken);

    QUrl url(apiBaseUrl + (path.startsWith("/") ? "" : "/") + path);

    if (!query.isEmpty()) {
        url.setQuery(QUrlQuery(query));
    }

    apiRequest.setUrl(url);

    QNetworkReply* reply = manager.sendCustomRequest(apiRequest, method.toUtf8());

    // auth required ?
    bool ok = exec(reply, retDatas);
    if (!ok && retDatas.value("networkError").toInt() == 401 ) { // Unauthorized = 401
        reply->blockSignals(true);
        reply->deleteLater();
        authorized = false;
        return sendRequest(path, query, method, retDatas);
    }
    return ok;
}

bool RHAPIClient::sendRequest(const QString& path, const QVariantMap& datas, const QString& method, QVariantMap& retDatas)
{
    if (!authorized && !auth(retDatas)) {
        return false;
    }

    QNetworkRequest apiRequest;
    apiRequest.setRawHeader("Authorization", QByteArray("Bearer ") + apiToken);

    QUrl url(apiBaseUrl + (path.startsWith("/") ? "" : "/") + path);
    apiRequest.setUrl(url);

    QNetworkReply* reply;

    if (!datas.isEmpty()) {
        QByteArray d = QJsonDocument::fromVariant(datas).toJson(QJsonDocument::Compact);
        apiRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        if (method == "POST") {
            reply = manager.post(apiRequest, d);
        }
        else if (method == "PUT") {
            reply = manager.put(apiRequest, d);
        }
        else {
            qWarning() << method + " n'est pas utilisé pour transmettre des données.";
            return false;
        }
    }
    else {
        reply = manager.sendCustomRequest(apiRequest, method.toUtf8());
    }

    // auth required ?
    bool ok = exec(reply, retDatas);
    if (!ok && retDatas.value("networkError").toInt() == 401 ) { //Unauthorize = 401,
        reply->blockSignals(true);
        reply->deleteLater();
        authorized = false;
        return sendRequest(path, datas, method, retDatas);
    }
    return ok;
}

bool RHAPIClient::exec(QNetworkReply* reply, QVariantMap& retDatas )
{
    QEventLoop loop;
    bool ok = true;

    QObject::connect(reply,
                     static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                     this, [this, &loop, &ok, reply, &retDatas](QNetworkReply::NetworkError code) {
        qWarning("Erreur requête %s %d : %s", qPrintable(reply->url().toString()), code, qPrintable(reply->errorString()));
        QString message;
        if (reply->isReadable()) {
            retDatas = QJsonDocument::fromJson(reply->readAll()).toVariant().toMap(); // on suppose doc.isObject()
            message = retDatas.value("userMessage").toString();
            if (message.isEmpty()) {
                message = reply->errorString();
            }
        }
        else {
            message = "Déconnexion réseau.";
            retDatas.insert("userMessage", message);
        }

        if (!message.isEmpty()) {
            qWarning() << message;
        }

        if (code == QNetworkReply::NetworkSessionFailedError ||
                code == QNetworkReply::HostNotFoundError ||
                code == QNetworkReply::UnknownNetworkError) {
            qWarning() << message;
        }

        ok = false;
    });

    connect(reply, &QNetworkReply::finished, this, [this, &loop, &ok, reply, &retDatas]() {
        if (!ok || !reply->isReadable()) {
            ok = false;
            loop.exit();
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        ok = !doc.isNull();
        // doc est null si un readAll a déjà eu lieu en retour erreur
        // dans ce cas on laisse la description de l'erreur dans datas
        if (ok && doc.isObject()) {
            retDatas = doc.toVariant().toMap();
        }
        loop.exit();
    });

    loop.exec();
    reply->blockSignals(true);
    reply->deleteLater();
    return ok;
}

// CRUD Methods

bool RHAPIClient::post(const QString& path, const QVariantMap& datas, QVariantMap &retDatas)
{
    return sendRequest(path, datas, "POST", retDatas);
}

bool RHAPIClient::get(const QString& path, QVariantMap &retDatas)
{
    return sendRequest(path, "", "GET", retDatas);
}

bool RHAPIClient::get(const QString& path, const QList<QPair<QString, QString>>& queryItems, QVariantMap &retDatas)
{
    QUrlQuery query;
    query.setQueryItems(queryItems);
    return sendRequest(path, query.toString(), "GET", retDatas);
}

bool RHAPIClient::put(const QString& path, const QVariantMap& datas, QVariantMap &retDatas)
{
    return sendRequest(path, datas, "PUT", retDatas);
}

bool RHAPIClient::del(const QString& path, QVariantMap &retDatas)
{
    return sendRequest(path, "", "DELETE", retDatas);
}
