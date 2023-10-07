#ifndef MAINUI_H
#define MAINUI_H

#include <QWidget>
#include <QCloseEvent>
#include <QDebug>

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

namespace Ui {
class mainUI;
}


class mainUI : public QWidget
{
    Q_OBJECT

public:
    //explicit mainUI(QWidget *parent = 0,QString username);
    mainUI(QWidget *parent,QString name);
    ~mainUI();
    void closeEvent(QCloseEvent *event) override;
    void showFriendList(int n,const char * p);
    void showMessageList(const char *p);
private:
    Ui::mainUI *ui;
    QString username;
signals:
    void showlogin();
    void closeLogin();
    void message_all(char *,int);
    void message_1v1();
private slots:
    void on_pushButton_seed_clicked();


};

#endif // MAINUI_H
