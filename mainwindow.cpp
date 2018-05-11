#include "mainwindow.h"

#include <QApplication>
#include <QPushButton>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    QPushButton* pushButton = new QPushButton(this);
    this->setCentralWidget(pushButton);
    pushButton->setText("Test RHAPIClient");
    connect(pushButton, SIGNAL(clicked()), this, SLOT(test()));
    client = new RHAPIClient("https://auth-dev.rhapi.net", "bXlhcHA6bXlhcHBteWFwcA", this);
    QVariantMap error;
    if (!client->auth("masteruser", "masteruser", error)) {
        QMessageBox::warning(this, "AUTH erreur", error.value("userMessage").toString());
    }
}

void MainWindow::test()
{
    qDebug() << "--- Tests : création + suppression d'une fiche patient + requête CCAM ---";
    qDebug() << QDateTime::currentDateTime();
    QVariantMap patient, datas;
    patient.insert("nom", "DUPONT");
    patient.insert("prenom", "PIERRE");
    patient.insert("naissance", "1980-03-20");
    if (client->post("Patients", patient, datas)) {
        qDebug() << "Création OK";
        QString id = datas.value("id").toString();

        qDebug() << id;
        if (id.isEmpty()) return;

        if (client->get("Patients/" + id, datas)) {
            qDebug() << "Lecture OK";
            qDebug() << datas;
        }
        else {
            QMessageBox::warning(this, "API erreur", datas.value("userMessage").toString());
        }

        if (client->get("Patients/" + id + "/age", datas)) {
            qDebug() << "Lecture de l'âge OK";
            qDebug() << datas;
        }
        else {
            QMessageBox::warning(this, "API erreur", datas.value("userMessage").toString());
        }

        if (client->del("Patients/" + id, datas)) {
            qDebug() << "Suppression OK";
            qDebug() << datas;
        }
        else {
            QMessageBox::warning(this, "API erreur", datas.value("userMessage").toString());
        }

        if (!client->get("Patients/" + id, datas)) {
            qDebug() << datas;
            qDebug() << QString() + "OK (retourne une erreur car le patient " + id + " vient d'être supprimé...";
        }
        else {
            QMessageBox::warning(this, "API erreur", "La fiche patient n'a pas été supprimée.");
        }

        QList<QPair<QString, QString>> params;
        params << (qMakePair(QString("texte"), QString("biopsie")));
        if (client->get("CCAM", params, datas)) {
            qDebug() << datas;
        }
        else {
            QMessageBox::warning(this, "API erreur", datas.value("userMessage").toString());
        }

    }
    else {
        QMessageBox::warning(this, "API erreur", datas.value("userMessage").toString());
    }
}

