#include "server.h"
#include "ui_server.h"
#include <QThread>
server::server(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::server)
{

    ui->setupUi(this);
    //********************數據庫操作*******************

    //设置数据库类型 ->我们用的是  sqlite3数据库
    db = QSqlDatabase::addDatabase("QSQLITE");

    //设置数据库文件的路径名
    db.setDatabaseName("/mnt/hgfs/Qt/QT project/userinfo_sky.db");

    //打开数据库文件(连接数据库)  //成功返回 true，失败返回false
    if(db.open())
    {
        //QMessageBox::critical(this,"","密码错误!");
        qDebug() << "sqlite_path_OK";
    }
    else
    {
        //QMessageBox::information(this,"正确","加油");
        qDebug() << "sqlite_path_ERROR";
    }
    //********************數據庫操作*******************
    tcpServer = new QTcpServer(this);
    tcpServer->listen(QHostAddress::Any,1234);//进入监听状态
    connect(tcpServer,SIGNAL(newConnection()),this,SLOT(on_newConnection()));

}

server::~server()
{
    db.close();
    delete ui;
}
void server::on_newConnection()
{
    qDebug()<< "server_connect_success";
    //只要进入该函数，就说明有客户端在请求连接
    QTcpSocket* socket = tcpServer->nextPendingConnection();//返回待处理的套接字，之后就可以用 socket和连接的客户端
                                                 //进行通信。先不考虑多个客户端
    socketList.push_back(socket);      //加入线程池

    //创建一个线程，用于和当前客户端的通信
    Skythread * t = new Skythread(socket,db);
    connect(t,SIGNAL(loginSuccess(QString,QTcpSocket *)),this,SLOT(on_loginSuccess(QString,QTcpSocket *)));
    connect(t,SIGNAL(disconnect(Skythread *,QTcpSocket *)),this,SLOT(on_disconnect(Skythread*,QTcpSocket*)));
    connect(t,SIGNAL(allmessage(QString,QString,QTcpSocket *)),this,SLOT(on_allmessage(QString,QString,QTcpSocket *)));//连接群聊消息函数
    t->start();
    qDebug()<< "有客户端连接成功";
}




void server::on_loginSuccess(QString name,QTcpSocket *s)
{
    ui->listWidget->addItem(name+"登录成功了");
    //保存登录成功的客户端的用户名及套接字
    map.insert(name,s);

    //向所有登录成功的客户端发送在线好友列表
    char buf[200];
    memset(buf,0,200);
    buf[0] = CMD_CHAT_LIST;
    buf[1] = map.size();  //返回map中键值对的数量(即在线客户端数量)
    qDebug() <<"在线客户端数量："<<map.size();
    int len;
    int seek = 2;
    //把好友列表保存到buf中
    qDebug() << map.size();
    for(QMap<QString,QTcpSocket *>::iterator it = map.begin();it!=map.end();++it)  // key值对于用户名，value对应套接字
    {
        qDebug() << it.key();
        len = it.key().toUtf8().toStdString().size();   //用户名字符串长度
        strncpy(buf+seek,it.key().toUtf8().toStdString().c_str(),len);
        seek += len+1;
    }
    qDebug() <<"LIST字节数[]："<< seek;
    qDebug()<<"用户名长度："<< len;
    //依次发送
    for(QMap<QString,QTcpSocket *>::iterator it = map.begin();it!=map.end();++it)
    {
        qDebug() <<"*****************in chat:"<<name;   //"\b"是一个特殊的转义字符，代表退格（Backspace）
                                        //"\u"是一个Unicode转义序列的开始
        send_one_data_package(it.value(),buf,seek);
    }
}
/*这段代码是一个服务器程序中的一个槽函数，用于处理客户端断开连接的事件。具体注释如下：

退出线程的事件循环，即停止线程的执行。
等待线程执行完毕，确保线程已经结束。
输出调试信息"delete"。
删除线程对象，释放内存。
在map中查找与断开连接的socket对应的用户名。
在UI界面的listWidget中添加退出群聊的提示消息。
从map中删除断开连接的socket。
准备发送在线好友列表的数据包。
把好友列表保存到buf中，buf的前两个字节分别表示数据包类型和在线好友的数量。
向所有登录成功的客户端发送在线好友列表。
*/
void server::on_disconnect(Skythread* t, QTcpSocket* s)
{
    // 退出线程的事件循环
    t->exit();
    // 等待线程执行完毕
    t->wait();

    qDebug() << "delete";
    // 删除线程对象
    delete t;

    // 在map中查找与断开连接的socket对应的用户名
    QMap<QString, QTcpSocket*>::iterator it;
    for (it = map.begin(); it != map.end(); ++it)
    {
        if (it.value() == s)
        {
            break;
        }
    }

    // 在UI界面的listWidget中添加退出群聊的提示消息
    ui->listWidget->addItem(it.key() + "退出群聊了");

    // 从map中删除断开连接的socket
    map.erase(it);

    // 向所有登录成功的客户端发送在线好友列表
    char buf[200];
    memset(buf, 0, 200);
    buf[0] = CMD_CHAT_LIST;
    buf[1] = map.size();
    int len;
    int seek = 2;

    // 把好友列表保存到buf中
    qDebug() <<"在线人数："<< map.size();
    for (QMap<QString, QTcpSocket*>::iterator it = map.begin(); it != map.end(); ++it)
    {
        qDebug() <<"chat_key:"<< it.key();
        len = it.key().toUtf8().toStdString().size();
        strncpy(buf + seek, it.key().toUtf8().toStdString().c_str(), len);
        seek += len + 1;
    }
    qDebug() <<"quit字节数:"<< seek;

    // 依次向所有在线客户端发送好友列表
    for (QMap<QString, QTcpSocket*>::iterator it = map.begin(); it != map.end(); ++it)
    {
        qDebug() <<"*****************in quit_chat";   //"\b"是一个特殊的转义字符，代表退格（Backspace）
                                        //"\u"是一个Unicode转义序列的开始
        send_one_data_package(it.value(), buf, seek);
    }
}

void server::on_allmessage(QString username, QString message, QTcpSocket *)
{
    // 在UI界面的listWidget中添加退出群聊的提示消息
   // ui->listWidget->addItem(it.key() + "退出群聊了");
    char buf[200];
    memset(buf, 0, 200);
    buf[0] = CMD_CHAT_MESSAGE_ALL;
    strncpy(buf+1, username.toStdString().c_str(), 20);
    strncpy(buf+21, message.toStdString().c_str(), 100);
    int seek = 121;
    // 依次向所有在线客户端发送群聊信息
    for (QMap<QString, QTcpSocket*>::iterator it = map.begin(); it != map.end(); ++it)
    {
        qDebug() <<"*****************in allmessage_chat";   //"\b"是一个特殊的转义字符，代表退格（Backspace）
                                        //"\u"是一个Unicode转义序列的开始
        send_one_data_package(it.value(), buf, seek);
    }
}

void server::on_readyRead()
{
    //QThread::sleep(10);
    //进到该函数就说明对方发送了数据过来
    //QString x = socket->read(100).toStdString().c_str();//最多读取100个字节
    /*
        readyRead的触发机制是 边缘触发，意味着不是有数据就会触发该信号
        那么我们必须在每次触发该信号时都要把缓冲区中的数据读取完毕
    */
    while(socketME->bytesAvailable()!=0)//意味着缓冲区中还有数据，继续读取
    {
        qDebug() << "conecting";
        char buf[256];
        memset(buf,0,256);   //
        int size = recv_one_data_package(socketME,buf,256);
        QString temp(buf);
        QString temp1(buf+21);
        qDebug() <<"raw_buf:"<< temp<<" / "<<temp1;
        qDebug() <<"size:"<< size;
        handle_connection(buf,size);

        //qDebug() << size;
        //QString x(buf);
        //qDebug() << x;
        //ui->listWidget->addItem(x);
    }
}



void server::on_pushButton_clicked(QTcpSocket * socket)
{
    QString x = ui->lineEdit->text();

    QListWidgetItem *aitem = new QListWidgetItem;
    aitem->setText(x);
    aitem->setTextAlignment(2);//右对齐
    ui->listWidget->addItem(aitem);
    //socket->write(x.toUtf8());//发送数据
    //x.toUtf8().toStdString().c_str()：将QString类型的x转换为UTF-8编码的std::string类型，并使用c_str()函数将其转换为const char*类型
    send_one_data_package(socket,x.toUtf8().toStdString().c_str(),x.toUtf8().toStdString().size());
    qDebug() << x.toUtf8().toStdString().c_str();
    ui->lineEdit->clear();
}

//读取一个数据包，p指向的数组用来保存读取到的原始数据包，size是数组p的大小，防止越界
/*
    收到数据之后需要转换回去，规则是：
    遇到0xfd 0xfd，就转换为 0xfd
    遇到0xfd 0x7f，就转换为 0x7f
    遇到单独的0x7f 说明是包头或者包尾
*/
int server::recv_one_data_package(QTcpSocket * socket,char * p,int size)
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


/*
    发送一个经过处理的数据包 p原始数据包的首地址，size是原始数据包的大小

    处理规则：
        如果用户原始数据中本来有0x7f，那么就把它变为  0xfd 0x7f
        如果用户原始数据中本来有0xfd，那么就把它变为  0xfd 0xfd
*/
void server::send_one_data_package(QTcpSocket * socket,const char *p,int size)
{
    //极端情况下（原始数据中全是0xff或0xfd），转换之后是转换之前的两倍
    char * r = new char[2*size];//分配两倍空间

    //开始转换
    int i;
    int j=0;
    for(i=0;i<size;i++)
    {
        if(p[i] == 0x7f)//是0xff,转换为0xfd 0xff
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
    q[0] = HEAD;     //添加帧头
    memcpy(q+1,r,j);  //内容
    q[j+1] = TAIL;    //添加帧尾

    QString temp(r+21);
    qDebug()<<"seed_data:"<<temp;
    socket->write(q,j+2);   //发送数据帧
    delete r;
    delete q;
}

void server::handle_connection(char* buf,int size)
{

    int cmd_no = buf[0];//接收帧 ，1个命令号，4个参数长度，参数内容
    qDebug() << "handle_cmd_no:"<<cmd_no;
    switch(cmd_no)
    {
        case CMD_LOGIN:handle_cmd_login(buf+1);break;
        case CMD_REG:handle_cmd_reg(buf+1);break;

        default:
                break;
    }

}

void server::handle_cmd(unsigned char *cmd, int len)
{
    //buf = buf +

}

void server::handle_cmd_ls()
{

}

void server::handle_cmd_get()
{

}

void server::handle_cmd_put()
{

}

void server::handle_cmd_bye()
{

}
/*
 *
*/
void server::handle_cmd_login(char* data)
{

    QString username(data);
    QString password(data+20);
    //qDebug() <<"handle_cmd"<< username<<" / "<<password;

    /*
     * 声明了 char *buf，但未将其初始化为任何值。因此，它的值是未定义的，可能会包含任何随机值。
    */
    //char *buf;     error
    // 分配一个足够大的缓冲区来保存命令和错误信息
    char buf[2];

    // 初始化缓冲区以避免未定义行为
    memset(buf, 0, sizeof(buf));
    buf[0] = CMD_LOGIN;

    if(username.isEmpty() || password.isEmpty())
    {

        buf[1] = UORPNULL_ERROR;
        //send_one_data_package(buf,2);
        qDebug()<<"ALL EMPTY";
    }
    else
    {
        QString sql = QString("select * from user_info where username='%1';").arg(username);
        qDebug() << sql;
        QSqlQuery query(db);
        //查询结果，要么是0条要么是1条记录
        query.exec(sql);
        if(query.next())
        {
            //查到了，需要判断密码是否匹配
            QString ps = query.value("passwd").toString();//获取查询到的记录的 passwd字段的值
            if(ps == password)//匹配成功
            {
                //登录成功

                buf[1] = LOGIN_OKK;
                qDebug() << "LOGIN SUCCESS";

            }
            else
            {
                buf[1] = PASSWORD_ERROR;
                qDebug() << "PASSWORD ERROR";
            }
        }
        else//没有查到
        {
            buf[1] = USERNAME_ERROR;
            qDebug() << "USERNAME_ERROR";
        }
    }


        send_one_data_package(socketME,buf,2);
}

void server::handle_cmd_reg(char* data)
{
    QString username(data);
    QString password(data+20);
    qDebug() <<"handle_cmd"<< username<<" / "<<password;
    QSqlQuery query(db);
    char buf[2];

    // 初始化缓冲区以避免未定义行为
    memset(buf, 0, sizeof(buf));
    buf[0] = CMD_REG;

    if(username.isEmpty() || password.isEmpty())
    {

        buf[1] = REG_UorP_NULL;
        //send_one_data_package(buf,2);
        qDebug()<<"ALL EMPTY";
    }
    else
    {
        QString sql = QString("insert into user_info values('%1','%2');").arg(username).arg(password);
       //执行sql语句
        if(!query.exec(sql))
        {
            buf[1] = REG_ERROR;
            qDebug() << "注册失败";
        }
        else
        {
            buf[1] = REG_OKK;
            qDebug() << "注册成功";
        }
    }

    send_one_data_package(socketME,buf,2);

}






