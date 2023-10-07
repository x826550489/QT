#ifndef THREAD_H
#define THREAD_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>
#include <QSqlQuery>
#include <QSqlDatabase>
#define HEAD 0x7f
#define TAIL 0x7f

typedef enum cmd_no
{
    CMD_LS = 0x01,
    CMD_GET = 0x02,
    CMD_PUT = 0x03,
    CMD_BYE = 0x04,
    CMD_MD5 = 0x05,
    CMD_LOGIN = 0X06,
    CMD_REG = 0X07,
    CMD_CHAT_LIST = 0X08,
    CMD_CHAT_MESSAGE_ALL,
    CMD_CHAT_MESSAGE_1V1,
    CMD_UNKNOWN = 0Xff
} cmd_no_t;



typedef enum login_message
{
    USERNAME_ERROR = 0X20,
    PASSWORD_ERROR,
    UORPNULL_ERROR,
    LOGIN_OKK,

    //...
} CMD_log;

typedef enum reg_message
{
    REG_OKK = 0X40,
    REG_UorP_NULL,
    REG_ERROR,
    //...
} REG_log;

class Skythread : public QThread
{
    Q_OBJECT
public:
    Skythread(QTcpSocket* s,QSqlDatabase &d);

    void send_one_data_package(const char *p, int size);
     int recv_one_data_package(char *p, int size);
    void signin(const char *p, int size);
    void login(const char *p, int size);
    void handle_connection(char* buf,int size);
public slots:
    void on_readyRead();
    void on_disconnected();

signals:
    void loginSuccess(QString name,QTcpSocket *s);
    void disconnect(Skythread * t,QTcpSocket *s);

    void allmessage(QString name,QString message,QTcpSocket *s);
protected:
    void run() override;


private:
    QTcpSocket * socket;
    QSqlDatabase &db;

    QString username;
};



#endif // THREAD_H
