#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "rhapiclient.h"

#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected slots:
    void test();
private:
    RHAPIClient* client;
};

#endif // MAINWINDOW_H
