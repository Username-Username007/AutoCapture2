#include "stdafx.h"
#include "AutoCapture.h"
#include <QtWidgets/QApplication>
#include "qDebug.h"
#include "qthread.h"
#include <qnetworkproxy.h>


class Client :public QThread
{
public:
    void run()override
    {
        qDebug() << QNetworkProxy::applicationProxy();
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
        qDebug() << QNetworkProxy::applicationProxy();
        QNetworkProxy::setApplicationProxy(QNetworkProxy::DefaultProxy);
        qDebug() << QNetworkProxy::applicationProxy();

        QTcpSocket socket;
        socket.connectToHost("127.0.0.1", 100);
        if (!socket.waitForConnected(1000))
        {
            qDebug() << "Client: Not connected";
            return;
        }
        auto proxy = socket.proxy();
        qDebug()<<"The proxy:"<<proxy<<proxy.hostName()<<proxy.port();
        socket.write("Hello!");
        if(!socket.waitForBytesWritten(1000))
		{
			qDebug() << "Client: Not written";
            return;
		}
        QTcpSocket s2;
        s2.connectToHost("127.0.0.2", 100);
        if (!s2.waitForConnected(1000))
		{
			qDebug() << "Client: Not connected";
			return;
		}
        s2.write("Hello! I'm socket2.");
        if (!s2.waitForBytesWritten(1000))
        {
            qDebug() << "Client: Not written";
			return;
        }
        if (!s2.waitForReadyRead(5000))
        {
            qDebug() << "Client: no read";
        }
        qDebug() << "Server's msg:" << s2.readAll();

    }
};
std::string getString()
{
    std::string str;
    char ch[200];
    qDebug()<<"ch:"<<(void*)ch;
    std::fill(ch, ch + 200, '\0');
    for (int i = 0; i < 199; i++)
    {
        ch[i] = std::rand()&0xff;
    }
    str = ch;
    qDebug()<<"str inside get string:"<<&str<<"str.c_str:"<<(void*)str.c_str();
    return std::move(str);
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    {
        Client client;
        client.start();
        QTcpServer server;
        server.listen(QHostAddress::Any, 100);
        qDebug()<< "Server: Listening";
        if (!server.waitForNewConnection(1000))
		{
			qDebug()<<"Server: Not connected";
            return -1;
		}
        QTcpSocket* pSocket = server.nextPendingConnection();
		qDebug() << "Server: Connected";
        if (!pSocket->waitForReadyRead(1000))
        {
            qDebug() << "Server: Not read";
			return -1;
        }
		QByteArray data = pSocket->readAll();
		qDebug() << "Client's msg:" << data;

        if (!server.waitForNewConnection(1000))
        {
            qDebug() << "Server: No new connection";
            return -1;
        }
        pSocket = server.nextPendingConnection();
        qDebug() << "Server: Connected";
        if (!pSocket->waitForReadyRead(1000))
		{
			qDebug() << "Server: Not read";
			return -1;
		}
        qDebug()<<"Client's msg:" << pSocket->readAll();
        pSocket->write("Hello, client 2, I'm the server");
        pSocket->flush();
        //pSocket->waitForBytesWritten(1000);
        client.wait();
    }

    {
        auto str1 = getString();
        std::string& str2 = (str1);
        qDebug() << "str1.c_str:" << (void*)str1.c_str() << "str2.c_str:" << (void*)str2.c_str();
        qDebug() << "str1:"<<&str1 << "str2:" << &str2;
        std::string newStr = std::move(str1);
        qDebug() << "str1.c_str:" << (void*)str1.c_str() << "newStr.c_str:" << (void*)newStr.c_str();
        qDebug() << "str1 size:" << str1.size();

    }
    {
        DataPacket p;
        auto pack = p.generateTcpPacket();
    }
    AutoCapture w;
    w.show();
    return a.exec();
}
