#pragma once
#include <QThread.h>
#include <qtreewidget.h>
#include <QList.h>
#include <qtcpsocket.h>
class FindServerWorker :public QThread
{
	Q_OBJECT
protected:
	const QStringList ips;
	const QVariantMap config;
	const int resultIndex;
	std::int32_t* result = nullptr;
public:
	FindServerWorker(const QStringList ips, const QVariantMap config, int resultIndex) :QThread(), ips(ips), config(config), resultIndex(resultIndex)
	{
		connect(this, &QThread::finished, this, &QObject::deleteLater);
		result = (std::int32_t*)config["D_serverStatus"].toLongLong();
	}
	void run()
	{
		int timeout = config["statusTimeout"].toInt()/4;
		int altPort = config["listenPort"].toInt();
		for (int i=0; i<ips.size(); i++)
		{
			auto ip = ips[i];
			QTcpSocket socket;
			socket.connectToHost(ip, 3324);
			if (!socket.waitForConnected(timeout))
			{
				socket.connectToHost(ip, altPort);
				if (!socket.waitForConnected(timeout))
				{
					//qDebug()<<ip<<" is not a server.";
					result[resultIndex+i] = 0;
					continue;
				}
			}
			socket.write(config["whatsYourStatus"].toByteArray());
			socket.waitForBytesWritten(timeout);
			socket.waitForReadyRead(timeout);
			auto resp = socket.readAll();
			qDebug()<<ip<<" replies:"<<QString::fromLatin1(resp);
			result[resultIndex + i] = *(std::int32_t*)resp.data();
		}
		
	}
	~FindServerWorker() { }

};
class FindServer :public QObject
{
	Q_OBJECT
public:
	// the parent must be the one with child "
	FindServer(QObject *pParent, const QVariantMap& config):QObject(pParent)
	{
		auto neighbors = pParent->findChild<QTreeWidget*>("twNetNeighborWidget");
		for (int i = 0; i < neighbors->topLevelItemCount(); i++)
		{
			serverIpList.append(neighbors->topLevelItem(i)->text(0));
		}
		for(int i = 0; i < serverIpList.size(); i+=2)
		{
			auto pThread = new FindServerWorker(serverIpList.mid(i, 2), config, i);
			pThread->start();
		}
	}
	~FindServer() {}

protected:
	QStringList serverIpList;
};