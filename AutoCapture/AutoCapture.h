#pragma once

#include <QtWidgets/QWidget>
#include "ui_AutoCapture.h"
#include "qlabel.h"
#include "qcamerainfo.h"
#include "qlayout.h"
#include "qtreewidget.h"
#include "Qlabel.h"
#include "qcameraviewfinder.h"
#include "CamViewfinder.h"
#include "qregularexpression.h"
#include <qtcpserver.h>
#include "QTCpSocket.h"
#include <qnetworkproxy.h>
#include <QCameraImageCapture>
#include <qabstractvideobuffer.h>
#include "DataPacket.hpp"
#include "FindServer.hpp"

class AutoCapture : public QWidget
{
    Q_OBJECT

public:
    AutoCapture(QWidget *parent = nullptr);
    ~AutoCapture();
    void readConfigurations();
    void toLog(const QString& msg, int level = 0);   // -1:green, 0:black, 1: yellow, 2: red
public slots:
    void onDisconnectFromServer();
    void updateNetNeighbor();
    void onTcpServerClosed();
    void testSelfCamera();
    void getServerClient();
    void onServerClientChange(QAbstractButton *btn);
    void startListen();
    void connectToServer();
    void serverNewData();       // the server gets new data from an existing connection
    void serverNewConnection(); // the server gets a new connection
    void onSocketClosed();
    void customSend();
    void onNetNeighborSelected(QTreeWidgetItem* pItem, int column);
    void onCollectServerStatus();
    void onSendTestImage(int id, const QVideoFrame& frame);
private:
    Ui::AutoCaptureClass ui;
protected:
    int m_packetReceivedCount = 0;
    QList<QWidget*> m_serverOnlyWidgets;
    QTextEdit* m_pTeLog = nullptr;
    QList<QWidget*> m_clientOnlyWidgets;
    QVariantMap m_appConfig;
    bool m_isServer = false;
    QList<QTcpSocket*> m_peerServers;    // the client's server, used for client, one client can connect to many servers
    QTcpSocket *m_pPeerClient=nullptr;    // the server's client, used for server
    QTcpServer *m_pTcpServer=nullptr;
    QMap<int, QString> mapLevelColor=QMap<int, QString>(
        {
            std::pair<int, QString>(-1, "green"),
            std::pair<int, QString>(0, "black"),
            std::pair<int, QString>(1, "rgb(248, 220, 65)"),
            std::pair<int, QString>(2, "red")
        });
    bool m_isReceivingPacket = false;
    DataPacket m_packetReceiving;       // clear it as soon as you have handled it and handle it as soon as you have received it fully.
    bool m_isSendingTestFrame = false;
    QQueue<QByteArray*> m_sendQueue;
protected:
    void handleData();
public:
};