#ifndef SERVER_H
#define SERVER_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>

#include <QDebug>
#include <QString>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include "sky_thread.h"
#define HEAD 0x7f
#define TAIL 0x7f


namespace Ui {
class server;
}

class server : public QWidget
{
    Q_OBJECT

public:
    explicit server(QWidget *parent = 0);
    ~server();

    int recv_one_data_package(QTcpSocket *,char *p, int size);
    void send_one_data_package(QTcpSocket *,const char *p, int size);
    void handle_connection(char* buf,int size);

protected:
    void handle_cmd(unsigned char* cmd, int len);
    void handle_cmd_ls();
    void handle_cmd_get();
    void handle_cmd_put();
    void handle_cmd_bye();
    void handle_cmd_login(char* data);
    void handle_cmd_reg(char* data);

public slots:
    void on_newConnection();
    void on_readyRead();
   // void on_disconnected();

    void on_loginSuccess(QString,QTcpSocket *);
    void on_disconnect(Skythread*,QTcpSocket*);
    void on_allmessage(QString,QString,QTcpSocket *);

private slots:
    void on_pushButton_clicked(QTcpSocket * socket);

private:
    Ui::server *ui;
    QTcpServer * tcpServer;
    QTcpSocket * socketME;
    QSqlDatabase db;  //存储用户数字数据库

    QList<QTcpSocket *>  socketList;//保存所有与服务端建立连接的套接字
    QMap<QString,QTcpSocket *> map; //保存套接字和对应用户名

protected:

};

#endif // SERVER_H
