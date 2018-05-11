#ifndef RHAPICLIENT_H
#define RHAPICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>

class RHAPIClient : public QObject
{
    Q_OBJECT
public:
    explicit RHAPIClient(const QString& authBaseUrl, const QByteArray& appToken, QObject *parent = nullptr);

    bool auth(const QString& user, const QString& password, QVariantMap &error);

    // CRUD Methods
    bool post(const QString& path, const QVariantMap& datas, QVariantMap &retDatas);
    bool get(const QString& path, QVariantMap &retDatas);
    bool get(const QString& path, const QList<QPair<QString, QString>>& queryItems, QVariantMap& retDatas);
    bool put(const QString& path, const QVariantMap& datas, QVariantMap& retDatas);
    bool del(const QString& path, QVariantMap& retDatas);

    bool isAuthorized() { return this->authorized; }

private:
    bool auth(QVariantMap &error);

    bool sendRequest(const QString& path, const QString& query, const QString& method, QVariantMap& retDatas);
    bool sendRequest(const QString& path, const QVariantMap& datas, const QString& method, QVariantMap& retDatas);

    bool exec(QNetworkReply* reply, QVariantMap& retDatas);

    QString authBaseUrl = "";
    QByteArray appToken = "";
    QString apiBaseUrl = "";
    QByteArray apiToken = "";
    QString user = "";
    QString password = "";
    QNetworkAccessManager manager;
    bool authorized = false;
};

#endif // RHAPICLIENT_H
