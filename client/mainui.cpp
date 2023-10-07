#include "mainui.h"
#include "ui_mainui.h"
//#include "client.h"
mainUI::mainUI(QWidget *parent,QString name) :
    QWidget(parent),username(name),
    ui(new Ui::mainUI)
{
   //socketw
    ui->setupUi(this);
}

mainUI::~mainUI()
{
    delete ui;
}

void mainUI::closeEvent(QCloseEvent *event)
{
    //qDebug() << "close";
    //emit showlogin();
    emit closeLogin();
}
/*
 *  n  在线客户端数量
 *  p  在线客户端名字
*/
void mainUI::showFriendList(int n, const char *p)
{
    qDebug() << "---in_showlist_way";
    ui->listWidget_frlist->clear();
    //p是好友名字列表的首地址，并且每个名字中间用 '\0' 分隔的
    int seek = 0;
    int i;
    for(i=0;i<n;i++)
    {
        QString name(p+seek);
        ui->listWidget_frlist->addItem(name);
        seek += name.toUtf8().toStdString().size()+1;
    }
}

void mainUI::showMessageList(const char *p)
{
    qDebug() << "---in_showMessagelist_way";
    //ui->listWidget_message->clear();
    QString name(p);
    QString message(p+20);
    ui->listWidget_message->addItem(name+":"+message);
    // 滚动到文本的最后一部分
    ui->listWidget_message->scrollToBottom();  // 再滚动到底部


}


void mainUI::on_pushButton_seed_clicked()    //消息框信息发送
{
    QString message = ui->textEdit_message->toPlainText();   //获取文本框内容
    char buf[100];
    buf[0] = CMD_CHAT_MESSAGE_ALL;
    //x.toUtf8().toStdString().c_str()：将QString类型的x转换为UTF-8编码的std::string类型，并使用c_str()函数将其转换为const char*类型，以便发送
    //转换为UTF-8编码的std::string对象，然后获取它的C风格字符串（即它的内容） error
    strncpy(buf+1,username.toUtf8().toStdString().c_str(),20);
    strncpy(buf+21,message.toUtf8().toStdString().c_str(),50);

    QString test(buf);
    QString test1(buf+21);

    qDebug() << "USERNAME:"<<test;
    qDebug() << "MESSAGE:"<<test1;
    //使用信号与槽
    qDebug() << "seed_message_all:";
    emit  message_all(buf,71);
    ui->textEdit_message->clear();//setPlainText(""); //清空文本框
    //send_one_data_package(buf,71);
}
