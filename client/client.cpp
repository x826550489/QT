#include "client.h"
#include "ui_client.h"
#include <QListWidgetItem>
/*
 * @author:sky
 * 功能清单
 * 1.客户端登录注册服务端     ok
 * 2.多客户端登录(多线程)     ok
 * 3.服务端在线用户信息分发    ok
 * 4.群聊                   ok
 * 5.1V1单聊
 * 6.FTP文件传输 传输和下载
 * BUG清单
 * 1.多线程服务端，客户端显示其他在线客户端有滞后性（要等下一客户端登录or退出客户端进程操作才会刷新）
 * 解决思路：
 * 核心：服务端客户端有没有正确收发
 * 1.服务端有没有正常发送登录和退出信号，以及发出列表数据包
 * 服务端正常发出list数据包  ok
 * 客户端没有正常进入接收函数，没有接收到套接字可read信号，第二个未登录客户端退出，服务端进入quit重新发送list，第一个客户端接收到list，
 * 解决方案 ：加入套接字有数据可读就一直读循环判断“while(socket->bytesAvailable() != 0)”（
 * readyRead的触发机制是 边缘触发，意味着不是有数据就会触发该信号
        那么我们必须在每次触发该信号时都要把缓冲区中的数据读取完毕）
 *
*/
client::client(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::client)
{
    //username = NULL;
    ui->setupUi(this);
    socket = new QTcpSocket(this);
    socket->connectToHost("192.168.121.131",1234);
    connect(socket,SIGNAL(readyRead()),this,SLOT(on_readyRead()));
}

client::~client()
{
    delete ui;
}

void client::on_readyRead()
{
    //进到该函数就说明对方发送了数据过来
    //QString x = socket->read(100).toStdString().c_str();//最多读取100个字节
    /*
        readyRead的触发机制是 边缘触发，意味着不是有数据就会触发该信号
        那么我们必须在每次触发该信号时都要把缓冲区中的数据读取完毕
    */

    while(socket->bytesAvailable() != 0)
    {
    char buf[256];
    memset(buf,0,256);
    int size = recv_one_data_package(buf,256);
    qDebug()<< "rev_size:" << size;
    recv_cmd_handle(buf,size);
    }
    //ui->listWidget->addItem(x);
}

void client::on_show()
{
    show();
}

/*
void client::on_pushButton_clicked()
{
    QString x = ui->lineEdit->text();

    QListWidgetItem *aitem = new QListWidgetItem;
    aitem->setText(x);
    aitem->setTextAlignment(2);//右对齐
    ui->listWidget->addItem(aitem);
    //socket->write(x.toUtf8());//发送数据
    //qDebug() << x.toUtf8().toStdString().c_str() ;
    //qDebug() << x.toUtf8().toStdString().size();
    //qDebug() << x.size();//QStingt类型的 size返回字符个数，不是字符串所占字节数目


    send_one_data_package(x.toUtf8().toStdString().c_str(),x.toUtf8().toStdString().size());

    ui->lineEdit->clear();
}
*/


//读取一个数据包，p指向的数·组用来保存读取到的原始数据包，size是数组p的大小，防止越界
/*
    收到数据之后需要转换回去，规则是：
    遇到0xfd 0xfd，就转换为 0xfd
    遇到0xfd 0x7f，就转换为 0x7f
    遇到单独的0x7f 说明是包头或者包尾
*/
int client::recv_one_data_package(char * p,int size)
{
    char x;
    int i=0;
    do
    {
        socket->read(&x,1);
    } while (x != HEAD);//找到包头，下一个字节开始就是数据包的内容了

    i = 0;
    while(1)
    {
        socket->read(&x,1);

        if(x == 0xfd)//如果是 0xfd
        {
            socket->read(&x,1);
            if(x == 0xfd)//说了有两个连续的 0xfd ,需要转换为一个 0xfd
            {
                p[i++] = 0xfd;
            }
            else if(x == 0x7f)//说了有连续的 0xfd 0xff,需要转换为一个 0xff
            {
                p[i++] = 0x7f;
            }
        }
        else if(x == TAIL || i == size-1)//说明遇到包尾
        {
            break;
        }
        else//普通字符
        {
            p[i++] = x;
        }
    }
    return i;//返回实际的数据包的长度
}

void client::recv_cmd_handle(char *data, int size)
{
    /*
       在Qt中，"\u"是用来表示Unicode字符的转义序列的开头。它用于指定一个Unicode字符的十六进制值。
       例如，"\u41" 表示字符 "A"，因为 "A" 的Unicode码位是0x41。
    */
    QString x(data);
    qDebug() <<"rev_data:"<< x;
    if(data[0] == CMD_LOGIN)
    {
        switch(data[1])
        {
            case LOGIN_OKK:
                            QMessageBox::information(this,"成功","登录成功!");
                            //登录成功后，自动加入“群聊”，自动打开群聊界面

                            eva = new mainUI(0,username);

                            //传输群聊发送消息
                            //connect(eva,SIGNAL(message_all(char * p,int size)),this,SLOT(on_message_all(char * p,int size)));
                            connect(eva,SIGNAL(message_all(char *,int)),this,SLOT(on_message_all(char *,int)));
                            connect(eva,SIGNAL(closeLogin()),this,SLOT(on_show()));  //closeLogin
                            eva->show();
                            this->hide();
                            break;

            case USERNAME_ERROR:QMessageBox::information(this,"无此用户名","登录失败!");
                                break;

            case PASSWORD_ERROR:QMessageBox::information(this,"密码错误","登录失败!");
                                break;

            case UORPNULL_ERROR:QMessageBox::information(this,"帐号或者密码为空","登录失败!");
                                break;
        }
    }
    else if(data[0] == CMD_REG)
    {
        switch(data[1])
        {
            case REG_OKK:
                QMessageBox::information(this,"okk","注册成功!");break;
            case REG_UorP_NULL:
                QMessageBox::information(this,"error","用户名或者密码输入为空!");break;
            case REG_ERROR:
                QMessageBox::information(this,"error","注册失败!");break;
        }

    }
    else if(data[0] == CMD_CHAT_LIST)
    {
        QString x1(data);
        qDebug() <<"in CMD_CHAT_LIST"<<"rev_data:"<<x1;
        eva->showFriendList(data[1],&data[2]);
    }
    else if(data[0] == CMD_CHAT_MESSAGE_ALL)
    {
        eva->showMessageList(&data[1]);     // 群聊消息

    }
}

void client::send_PandD(cmd_no_t cmd)
{
            username = ui->lineEdit_username->text();
    QString password = ui->lineEdit_password->text();
    char buf[41];
    buf[0] = cmd;

    //转换为UTF-8编码的std::string对象，然后获取它的C风格字符串（即它的内容）
    strncpy(buf+1,username.toUtf8().toStdString().c_str(),20);
    strncpy(buf+21,password.toUtf8().toStdString().c_str(),20);

    QString test(buf);
    QString test1(buf+21);

    qDebug() << "USERNAME:"<<test;
    qDebug() << "PASSWORD:"<<test1;

    send_one_data_package(buf,41);
}


/*
    发送一个经过处理的数据包，p原始数据包的首地址，size是原始数据包的大小

    处理规则：
        如果用户原始数据中本来有0x7f，那么就把它变为  0xfd 0x7f
        如果用户原始数据中本来有0xfd，那么就把它变为  0xfd 0xfd
*/
void client::send_one_data_package(const char *p,int size)
{
    //极端情况下（原始数据中全是0x7f或0xfd），转换之后是转换之前的两倍
    char * r = new char[2*size];//分配两倍空间

    //开始转换
    int i;
    int j=0;
    for(i=0;i<size;i++)
    {
        if(p[i] == 0x7f)//是0x7f,转换为0xfd 0x7f
        {
            r[j++] = 0xfd;
            r[j++] = 0x7f;
        }
        else if(p[i] == 0xfd)//是0xfd ，转换为 0xfd 0xfd
        {
            r[j++] = 0xfd;
            r[j++] = 0xfd;
        }
        else
        {
            r[j++] =p[i];
        }
    }
    char * q = new char[j + 2];
    q[0] = HEAD;
    memcpy(q+1,r,j);
    q[j+1] = TAIL;

    socket->write(q,j+2);
    delete r;
    delete q;
}




void client::on_pushButton_signIn_clicked()
{
    send_PandD(CMD_LOGIN);
}

void client::on_pushButton_register_clicked()
{
    send_PandD(CMD_REG);
}

void client::on_message_all(char *p, int size)
{
    send_one_data_package(p,size);
}
