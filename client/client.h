#ifndef CLIENT_H
#define CLIENT_H

#include <QWidget>
#include <QTcpSocket>
#include <QDebug>
#include <QString>
#include <QMessageBox>

#include "mainui.h"
#define HEAD 0x7f
#define TAIL 0x7f

//extern QTcpSocket *socket;

namespace Ui {
class client;
}

class client : public QWidget
{
    Q_OBJECT

public:
    explicit client(QWidget *parent = 0);
    ~client();

    void send_one_data_package(const char *p, int size);
    void recv_cmd_handle(char *data,int size);
    void send_PandD(cmd_no_t cmd);
     int recv_one_data_package(char *p, int size);


public slots:
    void on_readyRead(); 
    void on_show();

private slots:
    void on_pushButton_signIn_clicked();
    void on_pushButton_register_clicked();

    void on_message_all(char *,int);

private:
    Ui::client *ui;
    QTcpSocket *socket;
    mainUI *eva;
    QString username;
};

#endif // CLIENT_H
