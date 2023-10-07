#include "sky_thread.h"

Skythread::Skythread(QTcpSocket* s,QSqlDatabase &d)
    :db(d)
{
    socket = s;
}
void Skythread::run()
{
    qDebug() << "run()";
    connect(socket,SIGNAL(readyRead()),this,SLOT(on_readyRead()));
    connect(socket,SIGNAL(disconnected()),this,SLOT(on_disconnected()));
    /*
     * 调用exec()函数的目的是让线程保持活跃状态，以便继续监听socket的readyRead()和disconnected()等事件。
    */
    exec();//不让线程结束，才能继续监听事件
}


void Skythread::on_readyRead()
{
    //QThread::sleep(10);
    //进到该函数就说明对方发送了数据过来
    //QString x = socket->read(100).toStdString().c_str();//最多读取100个字节
    /*
        readyRead的触发机制是 边缘触发，意味着不是有数据就会触发该信号
        那么我们必须在每次触发该信号时都要把缓冲区中的数据读取完毕
    */
    while(socket->bytesAvailable()!=0)//意味着缓冲区中还有数据，继续读取
    {
        qDebug() << "conecting";
        char buf[256];
        memset(buf,0,256);   // 将 buf 指向的内存的前256个字节的值都设置为0。
        int size = recv_one_data_package(buf,256);
        QString temp(buf);
        QString temp1(buf+21);
        qDebug() <<"[1]_buf:"<< temp<<" / "<<temp1;
        qDebug() <<"size:"<< size;
        handle_connection(buf,size);

        //qDebug() << size;
        //QString x(buf);
        //qDebug() << x;
        //ui->listWidget->addItem(x);
    }
}


void Skythread::handle_connection(char* buf,int size)
{

    int cmd_no = buf[0];//接收帧 ，1个命令号，4个参数长度，参数内容
    qDebug() << "handle_cmd_no:"<<cmd_no;
    switch(cmd_no)
    {
        case CMD_LOGIN:login(buf+1,size);break;
        case CMD_REG:signin(buf+1,size);break;
        //xth switch-case语句中case：后面接多个语句，需要加入“{}”; error
        case CMD_CHAT_MESSAGE_ALL:{
                                    QString message(buf+21);
                                    emit allmessage(username,message,socket);
                                    qDebug() << "in_message_all";
                                    break;}
        case CMD_CHAT_LIST:;break;
        default:    break;
    }

}

void Skythread::on_disconnected()
{
    qDebug() << "客户端断开连接了";
    emit disconnect(this,socket);
}



//读取一个数据包，p指向的数组用来保存读取到的原始数据包，size是数组p的大小，防止越界
/*
    收到数据之后需要转换回去，规则是：
    遇到0xfd 0xfd，就转换为 0xfd
    遇到0xfd 0x7f，就转换为 0x7f
    遇到单独的0x7f 说明是包头或者包尾
*/
int Skythread::recv_one_data_package(char * p,int size)
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
void Skythread::send_one_data_package(const char *p,int size)
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
    q[0] = HEAD;
    memcpy(q+1,r,j);
    q[j+1] = TAIL;

    socket->write(q,j+2);
    delete r;
    delete q;
}

/*
    服务端回复消息给客户端的数据包格式：
        类型(1字节) type  1 -》注册   2-》登录
        数据包格式：
           包头(1字节) 类型(1字节) 结果(1字节) 包尾(1字节)
*/

void Skythread::login(const char *data, int size)
{
    username = data;
    //username(data.toUtf8().constData());
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
                //emit loginSuccess(username,socket);   //发送登录成功信号到主进程   error
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


        send_one_data_package(buf,2);
        if(buf[1] == LOGIN_OKK)//匹配成功
        {

            emit loginSuccess(username,socket);   //发送登录成功信号到主进程
            //登录成功
            qDebug() << "登录成功信号";
        }

}

void Skythread::signin(const char *data, int size)
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

    send_one_data_package(buf,2);
}
