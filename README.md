# QtRHAPIClient

Un client RHAPI Qt/C++

*RHAPI is a RESTful Health API*


## Installation
Le client est constitué des 2 fichiers :
- rhapiclient.h
- rhapiclient.cpp
Le projet Qt .pro doit comporter les lignes suivantes
```
QT += network
SOURCES += rhapiclient.cpp
HEADERS += rhapiclient.h
```

## Usage
Voir l'exemple fourni et les tests sur mainwindow.cpp

```C++
// Exemple : requête CCAM sur un mot clef
//
RHAPIClient* client = new RHAPIClient("https://auth-dev.rhapi.net", "bXlhcHzTA6bXlhcH", this);
QVariantMap error;
if (client->auth("username", "YfdfR5g", error)) {
    QList<QPair<QString, QString>> params;
    params << (qMakePair(QString("texte"), QString("biopsie")));
    if (client->get("CCAM", params, datas)) {
        qDebug() << datas;
    }
    else {
        qWarning() << datas.value("userMessage").toString();
    }
}
else {
    qWarning() << error.value("userMessage").toString();
}
