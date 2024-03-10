#include "stdafx.h"
#include "AutoCapture.h"

AutoCapture::AutoCapture(QWidget *parent)
    : QWidget(parent)
{
	readConfigurations();
    ui.setupUi(this);
    auto cameras = QCameraInfo::availableCameras();
	auto layout = new QVBoxLayout(this);


	auto scl = new QHBoxLayout();	// Server Client Layout
	auto rbImServer = new QRadioButton(tr("我是服务器"), this);
	rbImServer->setObjectName("rbImServer");
	rbImServer->setChecked(true);
	auto rbImClient = new QRadioButton(tr("我是客户端"), this);
	rbImClient->setObjectName("rbImClient");
	scl->addWidget(rbImServer);
	scl->addWidget(rbImClient);
	auto bg = new QButtonGroup(this);
	bg->setObjectName("bgServerClient");
	connect(bg, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, &AutoCapture::onServerClientChange);
	bg->addButton(rbImServer);
	bg->addButton(rbImClient);
	scl->addStretch();
	layout->addLayout(scl);

	{
		auto line = new QFrame(this);
		line->setFrameStyle(QFrame::HLine | QFrame::Sunken);
		layout->addWidget(line);
		auto teLog = new QTextEdit(this);
		layout->addWidget(teLog);
		teLog->setReadOnly(true);
		teLog->setObjectName("teLog");
		this->m_pTeLog = teLog;

	}

	{
		auto line = new QFrame(this);
		line->setFrameStyle(QFrame::HLine | QFrame::Sunken);
		layout->addWidget(line);
	}


	auto cameraList = new QTreeWidget(this);
	cameraList->setObjectName("cameraListWidget");

	for (auto camera : cameras)
	{
		cameraList->addTopLevelItem(new QTreeWidgetItem(QStringList() << camera.deviceName() << camera.description()));
	}
	cameraList->setHeaderItem(new QTreeWidgetItem(QStringList() << tr("摄像头名称") << "摄像头描述"));
	cameraList->resize(cameraList->sizeHint());


	m_clientOnlyWidgets.append(cameraList);
	layout->addWidget(cameraList);
	auto dcl = new QHBoxLayout();
	auto lDefaultCamera = new QLabel(tr("默认摄像头"), this);
	m_clientOnlyWidgets.append(lDefaultCamera);
	dcl->addWidget(lDefaultCamera);
	auto leDefaultCamera = new QLineEdit(QCameraInfo::defaultCamera().description());
	m_clientOnlyWidgets.append(leDefaultCamera);
	dcl->addWidget(leDefaultCamera);
	layout->addLayout(dcl);
	auto pbStartCam = new QPushButton(tr("启动摄像头"), this);
	pbStartCam->setObjectName("pbStartCam");
	m_clientOnlyWidgets.append(pbStartCam);
	layout->addWidget(pbStartCam);

	auto pbSeePacketReceived = new QPushButton(tr("查看收到的数据包"), this);
	layout->addWidget(pbSeePacketReceived);
	connect(pbSeePacketReceived, &QPushButton::clicked, 
		[this]() {
			this->toLog(this->m_packetReceiving.getData().toHex(), 1);
		});

	pbStartCam->connect(pbStartCam, &QPushButton::clicked, this, &AutoCapture::testSelfCamera);


	{
		auto line = new QFrame(this);
		line->setFrameStyle(QFrame::HLine | QFrame::Sunken);
		layout->addWidget(line);
	}

	auto twNetNeighborWidget = new QTreeWidget(this);
	auto netNeighborLayout = new QHBoxLayout();
	//auto pbConnect = new QPushButton(tr("连接"), this);
	//m_clientOnlyWidgets.append(pbConnect);
	//pbConnect->connect(pbConnect, &QPushButton::clicked, this, &AutoCapture::getServerClient);
	//pbConnect->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
	//m_clientOnlyWidgets.append(pbConnect);
	auto lNetNeighbor = new QLabel(tr("网络邻居"), this);
	m_clientOnlyWidgets.append(lNetNeighbor);
	netNeighborLayout->addWidget(lNetNeighbor);
	twNetNeighborWidget->setObjectName("twNetNeighborWidget");
	m_clientOnlyWidgets.append(twNetNeighborWidget);
	connect(twNetNeighborWidget, &QTreeWidget::itemClicked, this, &AutoCapture::onNetNeighborSelected);
	netNeighborLayout->addWidget(twNetNeighborWidget);
	
	auto pbUpdateNetNeighbor = new QPushButton(tr("刷新"), this);
	pbUpdateNetNeighbor->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

	m_clientOnlyWidgets.append(pbUpdateNetNeighbor);
	pbUpdateNetNeighbor->connect(pbUpdateNetNeighbor, &QPushButton::clicked, this, &AutoCapture::updateNetNeighbor);
	netNeighborLayout->addWidget(pbUpdateNetNeighbor);
	{
		auto divLine = new QFrame(this);
		divLine->setFrameStyle(QFrame::VLine | QFrame::Sunken);
		netNeighborLayout->addWidget(divLine);
	}
	//netNeighborLayout->addWidget(pbConnect);
	layout->addLayout(netNeighborLayout);
	QProcess proc;
	proc.start("arp -a", QIODevice::Text | QIODevice::ReadOnly);
	proc.waitForFinished();
	//qDebug().noquote()<<QString::fromUtf8(proc.readAllStandardOutput());
	//proc.seek(0);
	auto line = QString::fromUtf8(proc.readLine());
	twNetNeighborWidget->addTopLevelItem(new QTreeWidgetItem(QStringList() << "localhost" << tr("本机") << tr("未知")));
	while (line.size())
	{
		QRegularExpression ipRe(R"(\d+.\d+.\d+.\d+)");
		QRegularExpression macRe(R"(([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2}))");
		auto ipMatch = ipRe.match(line);
		auto macMatch = macRe.match(line);
		if(ipMatch.hasMatch() && macMatch.hasMatch())
		{
			auto ip = ipMatch.captured(0);
			auto mac = macMatch.captured(0);
			twNetNeighborWidget->addTopLevelItem(new QTreeWidgetItem(QStringList() << ip << mac <<tr("未知")));
		}
		twNetNeighborWidget->setHeaderItem(new QTreeWidgetItem(QStringList() << tr("IP地址") << tr("MAC地址")<<("服务器状态")));
		//output += line;
		line = QString::fromUtf8(proc.readLine());
	}
	twNetNeighborWidget->hideColumn(1);
	{
		// get server status
		auto serverStatus = new int[twNetNeighborWidget->topLevelItemCount()];
		std::fill(serverStatus, serverStatus + twNetNeighborWidget->topLevelItemCount(), 0xabcdef11);
		m_appConfig["D_serverStatus"] = (qint64)serverStatus;
		qDebug() << "server status result poitner:" << serverStatus;
		m_appConfig["D_serverStatusSize"] = twNetNeighborWidget->topLevelItemCount();
		FindServer fs(this, m_appConfig);
		QTimer().singleShot(m_appConfig["statusTimeout"].toInt() * 1.5, this, &AutoCapture::onCollectServerStatus);
	}

	auto lConnectedServer = new QLabel(tr("已连接的\n服务器"), this);
	netNeighborLayout->addWidget(lConnectedServer);
	m_clientOnlyWidgets.append(lConnectedServer);
	auto twConnectedServer = new QTreeWidget(this);
	twConnectedServer->setObjectName("twConnectedServer");
	twConnectedServer->setHeaderItem(new QTreeWidgetItem(QStringList() << tr("IP地址") ));
	netNeighborLayout->addWidget(twConnectedServer);
	m_clientOnlyWidgets.append(twConnectedServer);
	auto pbDisconnectFromServer = new QPushButton(tr("与此服务器\n断开连接"), this);
	netNeighborLayout->addWidget(pbDisconnectFromServer);
	m_clientOnlyWidgets.append(pbDisconnectFromServer);
	pbDisconnectFromServer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
	connect(pbDisconnectFromServer, &QPushButton::clicked, this, &AutoCapture::onDisconnectFromServer);



	{
		auto divLine = new QFrame(this);
		divLine->setFrameStyle(QFrame::HLine | QFrame::Sunken);
		layout->addWidget(divLine);
		auto pbStartListen = new QPushButton(tr("开始监听"), this);
		pbStartListen->setObjectName("pbStartListen");
		connect(pbStartListen, &QPushButton::clicked, this, &AutoCapture::startListen);
		m_serverOnlyWidgets.append(pbStartListen);
		auto ssl = new QHBoxLayout();
		ssl->addWidget(pbStartListen);

		auto pbConnect = new QPushButton(tr("连接伺服器"), this);
		connect(pbConnect, &QPushButton::clicked, this, &AutoCapture::connectToServer);
		ssl->addWidget(pbConnect);
		auto lServerIp = new QLabel(tr("伺服器IP"), this);
		m_clientOnlyWidgets.append(lServerIp);
		ssl->addWidget(lServerIp);
		auto leServerIp = new QLineEdit(this);
		leServerIp->setObjectName("leServerIp");
		ssl->addWidget(leServerIp);
		m_clientOnlyWidgets.append(leServerIp);
		auto lServerPort = new QLabel(tr("伺服器端口"), this);
		//m_clientOnlyWidgets.append(lServerPort);
		ssl->addWidget(lServerPort);
		auto leServerPort = new QLineEdit(this);
		leServerPort->setText(m_appConfig["listenPort"].toString());
		//m_clientOnlyWidgets.append(leServerPort);
		leServerPort->setObjectName("leServerPort");
		ssl->setStretchFactor(leServerPort, 1);
		ssl->addWidget(leServerPort);

		m_clientOnlyWidgets.append(pbConnect);
		layout->addLayout(ssl);

	}
	{
		auto divLine = new QFrame(this);
		divLine->setFrameStyle(QFrame::HLine | QFrame::Sunken);
		layout->addWidget(divLine);
		auto csl = new QHBoxLayout();	// client send layout
		layout->addLayout(csl);

		auto pbSend = new QPushButton(tr("发送"), this);
		connect(pbSend, &QPushButton::clicked, this, &AutoCapture::customSend);
		m_clientOnlyWidgets.append(pbSend);
		csl->addWidget(pbSend);

		auto teSend = new QTextEdit(this);
		teSend->setObjectName("teSend");
		csl->addWidget(teSend);
		m_clientOnlyWidgets.append(teSend);

	
	}

	layout->addStretch();
	this->onServerClientChange(rbImServer);
	//{
	//	// auto update net neighbor
	//	auto timer = new QTimer(this);
	//	connect(timer, &QTimer::timeout, this, &AutoCapture::updateNetNeighbor);
	//	timer->start(m_appConfig["updateNetNeighborInterval"].toInt());
	//}

}

AutoCapture::~AutoCapture()
{}

void AutoCapture::testSelfCamera()
{
	if (!m_isSendingTestFrame)
	{
		if (!m_peerServers.size())
		{
			toLog(tr("无可用连接, 启动摄像头也没用!"), 1);
			return;
		}
		auto clw = findChild<QTreeWidget*>("cameraListWidget");
		auto selectedCam = clw->selectedItems();
		QCameraInfo camInfo;
		if (!selectedCam.size())
		{
			qDebug() << "No camera is selected, using the default one:" << (camInfo = QCameraInfo::defaultCamera()).description();
		}
		else
		{
			qDebug() << "The selected camera is:" << selectedCam[0]->text(1);
			for (auto cam : QCameraInfo::availableCameras())
			{
				if (cam.description() == selectedCam[0]->text(1))
				{
					camInfo = cam;
					break;
				}
			}
		}


		/*{
			auto camera = new QCamera(camInfo, this);
			auto viewfinder = new CamViewfinder();
			viewfinder->show();
			auto imageCapture = new QCameraImageCapture(camera, this);
			camera->setViewfinder(viewfinder);
			camera->setCaptureMode(QCamera::CaptureStillImage);
			camera->start();
			connect(viewfinder, &CamViewfinder::onClose, camera, &QCamera::unload);
		}*/
		auto camera = new QCamera(camInfo, this);
		camera->setCaptureMode(QCamera::CaptureStillImage);
		auto cameraCapture = new QCameraImageCapture(camera, this);
		cameraCapture->setObjectName("cameraCapture");
		if (!cameraCapture->isCaptureDestinationSupported(QCameraImageCapture::CaptureToBuffer))
		{
			toLog(tr("摄像头不支持捕获到缓冲区! (我也不着为啥)"), 2);
			camera->deleteLater();
			cameraCapture->deleteLater();
			return;
		}
		cameraCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
		camera->start();
		connect(cameraCapture, &QCameraImageCapture::imageAvailable, this, &AutoCapture::onSendTestImage);
		m_isSendingTestFrame = !m_isSendingTestFrame;
		auto testVideoDisplay = new QDialog(this);
		testVideoDisplay->setObjectName("testVideoDisplay");
		testVideoDisplay->show();
		//testVideoDisplay->setWindowFlag(Qt::Dialog, true);
		auto lFrame = new QLabel(testVideoDisplay);
		lFrame->show();
		lFrame->setObjectName("videoPlay");
		auto* timer = new QTimer(this);
		auto timerCaptureFunc = [this, camera, cameraCapture, timer]() {
			if (this->m_isSendingTestFrame)
			{
				cameraCapture->capture();
				//{
				//	this->m_isSendingTestFrame = false;
				//	this->toLog("Warning: The timer has been deleted for testing.", 1);
				//}
			}
			else
			{
				timer->deleteLater();
				camera->deleteLater();
				cameraCapture->deleteLater();
			}
			};
		connect(timer, &QTimer::timeout,timerCaptureFunc);
		timer->setInterval(1000);
		timer->start();
		auto pbStartCam = findChild<QPushButton*>("pbStartCam");
		pbStartCam->setText(tr("停止摄像头"));
		pbStartCam->setStyleSheet("background-color: red;");
	}
	else
	{
		auto testVideoDisplay = findChild<QDialog*>("testVideoDisplay");
		if (testVideoDisplay)
		{
			testVideoDisplay->deleteLater();
		}

		// current state: sending data (timer on, camera on)
		m_isSendingTestFrame = !m_isSendingTestFrame;
		auto pbStartCam = findChild<QPushButton*>("pbStartCam");
		pbStartCam->setText(tr("摄像头, 启动!"));
		pbStartCam->setStyleSheet("");
	}
	
}

void AutoCapture::onSendTestImage(int id, const QVideoFrame& frame)
{
	auto testVideoDisplay = findChild<QDialog*>("testVideoDisplay");
	auto image = frame.image().convertToFormat(QImage::Format_RGB888);
	if (testVideoDisplay)
	{
		//testVideoDisplay->show();
		auto videoPlay = testVideoDisplay->findChild<QLabel*>("videoPlay");
		videoPlay->setPixmap(QPixmap::fromImage(image));
		videoPlay->resize(videoPlay->sizeHint());
		testVideoDisplay->resize(videoPlay->sizeHint());
	}
	QByteArray dataPack;
	{
		DataPacket pack;
		QByteArray data;
		std::int32_t bytesPerLane = image.bytesPerLine();
		auto imageSize = image.size();
		std::int32_t width = imageSize.width();
		std::int32_t height = imageSize.height();
		data.append((char*)&width, sizeof(std::int32_t));
		data.append((char*)&height, sizeof(std::int32_t));
		data.append((char*)&bytesPerLane, sizeof(std::int32_t));
		data.append((char*)image.bits(), image.sizeInBytes());

		pack.setData(std::move(data), DataPacket::TestVideoFrame);
		dataPack = pack.generateTcpPacket(m_appConfig["binaryMessage"].toByteArray());
	}
	for (auto server : m_peerServers)
	{
		server->write(dataPack);
		server->flush();
	}



}

void AutoCapture::getServerClient()
{
	auto bg = findChild<QButtonGroup*>("bgServerClient");
	bool m_isServer = bg->checkedButton()->objectName() == "rbImServer";

	qDebug() << "Server state: " << m_isServer;
#if defined _WIN64
	QResource res(":/wincmd/setFirewall");

	QProcess proc(this);
	//proc.start(R"(powershell -ExecutionPolicy bypass -command "win_setFirewall.ps1")", QIODevice::Text | QIODevice::ReadOnly);
	proc.start("powershell", QStringList() << "-executionpolicy" << "bypass"
		<< "-command" << QString("&{") + QString::fromUtf8(res.uncompressedData()) + "}", QIODevice::Text | QIODevice::ReadOnly);
	//proc.start("powershell -ExecutionPolicy bypass -command &{$a=10;$b=10;Write-Host ($a+$b);}");
	proc.waitForFinished();
	qDebug().noquote() << QString::fromUtf8(proc.readLine());
#elif defined __linux
	QResource res(":/linuxcmd/setFirewall");
#else
//#error "Unknown OS"
#endif

	//qDebug().noquote() << QString::fromUtf8(res.uncompressedData());

}

void AutoCapture::onServerClientChange(QAbstractButton *btn)
{

	bool m_isServer = btn->objectName() == "rbImServer";
	qDebug() << "Server state: " << m_isServer;
	if (m_isServer == this->m_isServer)
	{
		return;
	}
	m_pTeLog->setText("");
	if (m_isServer)
	{
		// change TO server, but now it's still client.
		// check if connection exists
		if (m_peerServers.size())
		{
			switch (QMessageBox(QMessageBox::Warning, tr("注意"), tr("已有连接, 你确定继续切换状态吗? 若切换状态则会导致连接全部中断!"), QMessageBox::Yes | QMessageBox::No).exec())
			{
			case QMessageBox::Yes:
				for (auto socket : m_peerServers)
				{
					socket->disconnectFromHost();
				}
				break;
			case QMessageBox::No:
				sender()->blockSignals(true);
				findChild<QRadioButton*>("rbImClient")->setChecked(true);
				sender()->blockSignals(false);
				return;
				break;
			}

		}
		toLog(tr("已切换至服务器模式."), 1);
		for(auto widget:m_clientOnlyWidgets)
		{
			widget->setEnabled(false);
		}
		for(auto widget:m_serverOnlyWidgets)
		{
			widget->setEnabled(true);
		}
	}
	else
	{
		// change TO client, but now it's still server
		if (m_pTcpServer)
		{
			switch (QMessageBox(QMessageBox::Warning, tr("警告"), tr("你现在依然在监听/有连接, 继续切换状态会导致监听中断, 是否要继续?"), QMessageBox::Yes|QMessageBox::No, this).exec())
			{
			case QMessageBox::Yes:
				m_pTcpServer->close();
				delete m_pTcpServer;
				m_pTcpServer = nullptr;
				break;
			case QMessageBox::No:
				sender()->blockSignals(true);
				findChild<QRadioButton*>("rbImServer")->setChecked(true);
				sender()->blockSignals(false);
				return;
				break;
			}
		}
		toLog(tr("已切换至客户端模式."), 1);
		for (auto widget : m_clientOnlyWidgets)
		{
			widget->setEnabled(true);
		}
		for (auto widget : m_serverOnlyWidgets)
		{
			widget->setEnabled(false);
		}
	}
	this->m_isServer = m_isServer;

}

void AutoCapture::readConfigurations()
{

	QVariantMap config;
	QFile configFile("config.json");
	QResource defaultJson(":/AutoCapture/config.json");

	auto defaultConfig = QJsonDocument::fromJson(defaultJson.uncompressedData()).toVariant().toMap();
	if (configFile.open(QIODevice::ReadOnly))
	{		auto configJson = configFile.readAll();
		config = QJsonDocument::fromJson(configJson).toVariant().toMap();
		for (auto key : defaultConfig.keys())
		{
			if (config.contains(key))
			{
				m_appConfig[key] = config[key];
			}
			else
			{
				m_appConfig[key] = defaultConfig[key];
				QMessageBox(QMessageBox::Warning, tr("注意"), QString(tr("配置文件中缺少")) + key + tr("字段, 使用默认配置.")).exec();
			}
		}

	}
	else
	{
		QMessageBox(QMessageBox::Warning, tr("注意"), QString(tr("目录"))+QDir::currentPath()+tr("下不存在config.json文件, 使用默认配置.")).exec();
		qDebug() << "Failed to open config file";

		m_appConfig = defaultConfig;
		QFile configFile("config.json");
		configFile.open(QIODevice::WriteOnly);
		configFile.write(defaultJson.uncompressedData());
		
	}
	qDebug() << "Loaded configurations:";

	// transforming hex configs to QByteArray
	for (auto key : m_appConfig["hexConfigs"].toList())
	{
		m_appConfig[key.toString()] = QByteArray::fromHex(m_appConfig[key.toString()].toString().replace(" ", "").toUtf8());
	}
	
	for(auto key: m_appConfig["logLevelColor"].toMap().keys())
	{
		mapLevelColor[key.toInt()] = m_appConfig["logLevelColor"].toMap()[key].toString();
	}
	for (auto key: m_appConfig.keys())
	{
		if(m_appConfig[key].type() == QVariant::Type::ByteArray)
					{
			qDebug() <<'\t'<< key << ":" << m_appConfig[key].toByteArray().toHex();
		}
		else if(m_appConfig[key].type() == QVariant::Type::Map)
		{
			qDebug() <<'\t'<< key << ":" << m_appConfig[key].toMap();
		}
		else
		{
			qDebug() <<'\t'<< key << ":" << m_appConfig[key].toString();
		}

	}
}

void AutoCapture::startListen()
{
	// TODO: 在此处添加实现代码.
	if (!m_pTcpServer)
	{
		// no server
		QTcpServer* server = new QTcpServer(this);
		m_pTcpServer = server;
		connect(m_pTcpServer, &QTcpServer::destroyed, this, &AutoCapture::onTcpServerClosed);
		server->setObjectName("tcpServer");
		auto leServerPort = findChild<QLineEdit*>("leServerPort");
		if (!server->listen(QHostAddress::Any, leServerPort->text().toInt()))
		{
			// listening failed
			QMessageBox(QMessageBox::Critical, tr("错误"), tr("无法监听端口, 你无法正常使用!")).exec();
			server->deleteLater();
			m_pTcpServer = nullptr;
			toLog(QString(tr("无法监听端口%1, 你无法正常使用!")).arg(leServerPort->text().toInt()), 2);
		}
		else
		{
			// listening succeeded, implement the connect
			//QMessageBox(QMessageBox::Information, tr("注意"), QString(tr("监听端口%1成功! 请使用客户端进行连接.")).arg(m_appConfig["listenPort"].toInt())).exec();
			connect(server, &QTcpServer::newConnection, this, &AutoCapture::serverNewConnection);
			toLog(QString(tr("监听端口%1成功! 请使用客户端进行连接.")).arg(leServerPort->text().toInt()), -1);
		}
		auto pbStartListen = findChild<QPushButton*>("pbStartListen");
		pbStartListen->setText(tr("停止监听"));
		pbStartListen->setStyleSheet("background-color: red;");

	}
	else if(!m_pPeerClient)
	{
		// has serer but no client
		m_pTcpServer->close();
		delete m_pTcpServer;
		m_pTcpServer = nullptr;
	}
	else
	{
		// has server and client
		m_pPeerClient->disconnectFromHost();
		m_pPeerClient->deleteLater();
		m_pPeerClient = nullptr;	
	}
}

void AutoCapture::onTcpServerClosed()
{

	auto pbStartListen = findChild<QPushButton*>("pbStartListen");
	pbStartListen->setText(tr("开始监听"));
	toLog(QString(tr("已停止监听.")), 0);
	pbStartListen->setStyleSheet("");
}

void AutoCapture::connectToServer()
{
	auto leServerIp = findChild<QLineEdit*>("leServerIp");
	auto leServerPort = findChild<QLineEdit*>("leServerPort");
	bool noThisIpConnection = true;	// The server with the same IP is not connected
	for (auto socket : m_peerServers)
	{
		if (socket->peerAddress() == QHostAddress(leServerIp->text()))
		{
			noThisIpConnection = false;
			//toLog(tr("你已连接到该服务器! 无法重复连接!"), 1);
			break;
		}
	}
	if (noThisIpConnection)
	{
		auto socket = new QTcpSocket(this);
		//socket->connectToHost(m_appConfig["serverIP"].toString(), m_appConfig["serverPort"].toInt());

		socket->connectToHost(leServerIp->text(), leServerPort->text().toInt());
		if (!socket->waitForConnected(m_appConfig["connectTimeout"].toInt()))
		{
			//QMessageBox(QMessageBox::Critical, tr("错误"), tr("无法连接到服务器!")).exec();
			toLog(tr("连接请求超时!"), 2);
		}
		else
		{
			socket->write(m_appConfig["handshakeBits"].toByteArray());
			auto timeout = m_appConfig["connectTimeout"].toInt();
			socket->flush();
			if (socket->waitForReadyRead(timeout))
			{
				auto serverResponse = socket->read(4);
				if (serverResponse == m_appConfig["handshakeBits"].toByteArray())
				{
					// handshake succeeded
					//qDebug() << "Handshake succeeded, the client has a new connection.";
					toLog(tr("连接握手成功!"), -1);
					auto twConnectedServer = findChild<QTreeWidget*>("twConnectedServer");
					twConnectedServer->addTopLevelItem(new QTreeWidgetItem(QStringList() << socket->peerAddress().toString()));
					m_peerServers.append(socket);
					connect(socket, &QTcpSocket::disconnected, this, &AutoCapture::onSocketClosed);
				}
				else
				{
					// handshake failed
					//qDebug() << "The server refused to connect. The server says:" << serverResponse;
					serverResponse += socket->readAll();
					toLog(tr("伺服器拒绝了连接, 并回复:")+QString::fromLatin1(serverResponse), 2);
					socket->disconnectFromHost();
				}
			}
			else
			{
				// connect timeout
				//qDebug() << "Connection timeout";
				toLog(tr("发送握手请求后, 服务器未回复, 连接超时."), 2);
				socket->disconnectFromHost();
			}

		}
	}
	else
	{
		//QMessageBox(QMessageBox::Warning, tr("注意"), tr("已经连接到该服务器!")).exec();
		toLog(tr("你已连接到该服务器! 无法重复连接!"), 1);
	}
}

void AutoCapture::serverNewData()
{
	auto socket = qobject_cast<QTcpSocket*>(sender());
	if (!socket)
	{
		toLog(tr("槽函数收到了未知的消息信号(请忽略此信息)"), 1);
		return;
	}
	if (!m_pPeerClient)
	{
		// no connection
		auto handshakeBits = socket->read(4);
		qDebug()<< "Client says:" << handshakeBits;
		if(handshakeBits == m_appConfig["handshakeBits"].toByteArray())
		{
			// handshake succeeded
			m_pPeerClient = socket;
			qDebug() << "Handshake succeeded, the server has a new connection.";
			socket->write(m_appConfig["handshakeBits"].toByteArray());
			auto pbStartListen = findChild<QPushButton*>("pbStartListen");
			pbStartListen->setText(tr("停止连接: ")+socket->peerAddress().toString());
			pbStartListen->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
			pbStartListen->setStyleSheet("background-color: red;");
			toLog(tr("已成功连接至客户端: ")+socket->peerAddress().toString(), -1);
			connect(socket, &QTcpSocket::disconnected, this, &AutoCapture::onSocketClosed);
		}
		else if (handshakeBits == m_appConfig["whatsYourStatus"].toByteArray())
		{
			qDebug()<<"The client queries your status.";
			socket->write(m_appConfig["goodToConnect"].toByteArray());
			socket->waitForBytesWritten(m_appConfig["connectTimeout"].toInt());
			socket->disconnectFromHost();
			socket->deleteLater();

		}
		else
		{
			// handshake failed
			qDebug() << "Handshake failed, the connection is refused.";
			socket->write(m_appConfig["refuseConnectionBits"].toByteArray());
			socket->waitForBytesWritten(m_appConfig["connectTimeout"].toInt());
			{
				QString str;
				QTextStream out(&str);
				out << socket->peerAddress().toString() << ":" << handshakeBits << ". "<<tr("握手失败!");
				toLog(str, 2);
			}

			//toLog(tr("客户端:")+handshakeBits+tr(", 未正确握手, 终止连接"), 2);
			socket->disconnectFromHost();
			socket->deleteLater();
		}

	}
	else
	{
		// already has a connection
		if(socket == m_pPeerClient)
		{
			// the server already has the connection (expected)
			QByteArray clientResponse;
			if (!m_isReceivingPacket)
			{
				clientResponse = socket->read(4);
			}
			if (clientResponse == m_appConfig["binaryMessage"].toByteArray() || m_isReceivingPacket)
			{
				m_isReceivingPacket = true;
				// binary message
				bool newPack = false;
				if (m_packetReceiving.getDataType() == DataPacket::Invalid)
				{
					newPack = true;
					// this is the head of the data packet.
				}

				if (m_packetReceiving.appendFromStream(socket))
				{
					toLog(QString(tr("数据包(%1)传输完成!")).arg(m_packetReceivedCount), 0);
					m_packetReceivedCount++;
					m_isReceivingPacket = false;
					handleData();	// to be implemented
				}
				if (newPack)
				{
					toLog(QString(tr("正在接收新数据包(%1), %2字节.")).arg(m_packetReceivedCount).arg(m_packetReceiving.getExpectedSize()), 0);

				}
			}
			else
			{
				// utf-8 text message
				clientResponse += socket->readAll();
				//toLog(tr("客户端: ") + QString::fromUtf8(std::move(clientResponse)), 0);
				toLog(tr("客户端: ") + QString::fromUtf8(std::move(clientResponse)).midRef(0, 20), 0);
			}
		}
		else
		{
			// the server has another connection (not expected)
			auto clientResponse = socket->readAll();
			//qDebug() << "The server already has another connection.";
			{
				QString str;
				QTextStream out(&str);
				out << tr("有未知终端(") << socket->peerAddress().toString() <<') '<<tr("发送了")<< clientResponse;
				toLog(str, 2);
			}
			socket->write(m_appConfig["refuseConnectionBits"].toByteArray());
			socket->waitForBytesWritten(m_appConfig["connectTimeout"].toInt());
			socket->disconnectFromHost();
			socket->deleteLater();
		}
	}


}

// the data to be handled is in m_packetReceiving
void AutoCapture::handleData()
{
	switch (m_packetReceiving.getDataType())
	{
	case DataPacket::Invalid:
		toLog(QString(tr("第%1个数据包(数据长度:%2字节)无效数据")).arg(m_packetReceivedCount).arg(m_packetReceiving.size()), 2);
		break;
	case DataPacket::TestVideoFrame:
		{
			auto testVideoDisplay = findChild<QDialog*>("testVideoDisplay");
			if (!testVideoDisplay)
			{
				testVideoDisplay = new QDialog(this);
				testVideoDisplay->setObjectName("testVideoDisplay");
				auto lFrame = new QLabel(testVideoDisplay);
				lFrame->setObjectName("lFrame");
				testVideoDisplay->show();
			}
			testVideoDisplay->show();
			auto lFrame = testVideoDisplay->findChild<QLabel*>("lFrame");
			auto& pack = m_packetReceiving;
			auto buf = pack.getData().constData();
			std::int32_t width = *(std::int32_t*)buf;
			std::int32_t height = *(std::int32_t*)(buf + 4);
			std::int32_t bytesPerLane = *(std::int32_t*)(buf + 8);
		
				//img.fromData((const uchar*)buf + 12, width, height, QImage::Format_RGB888);
				QImage img((const uchar*)buf + 12, width, height, QImage::Format_RGB888);
				lFrame->setPixmap(QPixmap::fromImage(std::move(img)));
			lFrame->resize(lFrame->sizeHint());
			testVideoDisplay->resize(lFrame->sizeHint());
		}
		break;
	case DataPacket::Text:
		toLog(QString(tr("客户端: %1")).arg(QString::fromUtf8(std::move(m_packetReceiving.getData()))), 1);
		break;
	default:
		toLog(QString(tr("第%1个数据包(数据长度:%2字节)格式未知!")).arg(m_packetReceivedCount).arg(m_packetReceiving.size()), 2);
		break;
	}
	m_packetReceiving.clear();
}

void AutoCapture::serverNewConnection()
{
	auto server = m_pTcpServer;
	auto socket = server->nextPendingConnection();
	if (!m_pPeerClient)
	{
		// no connection
		connect(socket, &QTcpSocket::readyRead, this, &AutoCapture::serverNewData);
		//qDebug() << "New connection from:" << socket->peerAddress().toString();
		//toLog(QString(tr("新连接来自%1")).arg(socket->peerAddress().toString()), 0);
	}
	else
	{
		// the server already has the connection.
		qDebug() << "The server already has a connection.";		socket->waitForReadyRead(m_appConfig["connectTimeout"].toInt());
		auto clientResponse = socket->readAll();
		if (clientResponse == m_appConfig["whatsYourStatus"].toByteArray())
		{
			qDebug() << "Another client queries your status.";
			socket->write(m_appConfig["alreadyConnected"].toByteArray());
			socket->waitForBytesWritten(m_appConfig["connectTimeout"].toInt());

		}
		else
		{
			toLog(QString(tr("新连接来自%1: %2. 但已有连接, 将拒绝连接.")).arg(socket->peerAddress().toString()).arg(QString::fromLatin1(clientResponse)), 2);
			qDebug() << "Bytes written to refuse connection: " << socket->write(m_appConfig["refuseConnectionBits"].toByteArray());
			socket->waitForBytesWritten(m_appConfig["connectTimeout"].toInt());

		}
		socket->disconnectFromHost();
		socket->deleteLater();

	}

	//else
	//{
	//	auto socket = m_pTcpServer->nextPendingConnection();
	//	qDebug()<<socket->peerAddress()<<"is trying to connect, but the server already has a connection.";
	//	if (socket->isWritable())
	//	{
	//		socket->write(m_appConfig["refuseConnectionBits"].toByteArray());
	//	}
	//	else
	//	{
	//		qDebug() << "The socket is not writable, cannot send the refuse connection bits.";
	//	}
	//	socket->close();
	//}
}


void AutoCapture::onSocketClosed()
{
	if (m_isServer)
	{
		auto pbStartListen = findChild<QPushButton*>("pbStartListen");
		pbStartListen->setText(tr("停止监听"));
		toLog(tr("客户端已断开连接."), 1);
		m_pPeerClient->deleteLater();
		m_pPeerClient = nullptr;
	}
	else
	{
		auto pPeerServer = qobject_cast<QTcpSocket*>(sender());
		//toLog(QString(tr("服务器%1已断开连接.")).arg(pPeerServer->peerAddress().toString()), 1);
		auto twConnectedServer = findChild<QTreeWidget*>("twConnectedServer");
		auto losingSocket = twConnectedServer->findItems(pPeerServer->peerAddress().toString(), Qt::MatchExactly);
		if (losingSocket.size() == 0)
		{
			toLog(tr("正在断开连接的服务器")+pPeerServer->peerAddress().toString()+tr("似乎不在已连接的服务器列表中."), 2);
		}
		else if(losingSocket.size() > 1)
		{
			toLog(tr("已连接的服务器列表中有多个")+pPeerServer->peerAddress().toString()+tr("的服务器, 请检查!"), 2);
		}
		else
		{
			toLog(tr("已与") + pPeerServer->peerAddress().toString() + tr("断开连接."), 1);
			delete losingSocket[0];
		}


		m_peerServers.removeOne(pPeerServer);

		pPeerServer->deleteLater();
	}

}
void AutoCapture::toLog(const QString& msg, int level)
{
	if (msg.size() > 100)
	{
		qDebug() << "Error!";
	}
	m_pTeLog->append(QString("<h3 style='color:%1;margin:0px;'>%2</h3>").arg(mapLevelColor[level]).arg(msg));
}

void AutoCapture::customSend()
{
	int a = 0;
	for (auto socket : m_peerServers)
	{
		auto msg = findChild<QTextEdit*>("teSend")->toPlainText();
		socket->write(msg.toUtf8());
		toLog(tr("已发送:")+msg, 0);
	}
}


void AutoCapture::onNetNeighborSelected(QTreeWidgetItem *pItem, int column)
{
	auto leServerIp = findChild<QLineEdit*>("leServerIp");
	leServerIp->setText(pItem->text(0));
}

void AutoCapture::onCollectServerStatus()
{
	std::int32_t* result = (std::int32_t*)m_appConfig["D_serverStatus"].toLongLong();
	auto sz = m_appConfig["D_serverStatusSize"].toInt();
	auto twNetNeighborWidget = findChild<QTreeWidget*>("twNetNeighborWidget");
	for (int i = 0; i < sz; i++)
	{
		if(result[i] == *(std::int32_t*)(m_appConfig["alreadyConnected"].toByteArray().data()))
		{
			twNetNeighborWidget->topLevelItem(i)->setText(2,tr("1: 已有客户端"));
			twNetNeighborWidget->topLevelItem(i)->setTextColor(2, QColor(255/2, 255/2, 0));
		}
		else if (result[i] == *(std::int32_t*)(m_appConfig["goodToConnect"].toByteArray().data()))
		{
			twNetNeighborWidget->topLevelItem(i)->setText(2, tr("0: 可以连接"));
			twNetNeighborWidget->topLevelItem(i)->setTextColor(2, QColor(0, 255, 0));

		}
		else
		{
			twNetNeighborWidget->topLevelItem(i)->setTextColor(2, QColor(255, 0, 0));
			twNetNeighborWidget->topLevelItem(i)->setText(2, tr("2: 无服务器"));
		}
		twNetNeighborWidget->sortByColumn(2, Qt::SortOrder::AscendingOrder);

	}

	delete[] result;
	m_appConfig["D_serverStatus"] = 0;
}

void AutoCapture::updateNetNeighbor()
{
	if (m_appConfig["D_serverStatus"].toInt())
	{
		// the server status is being updated, do not update again.
		return;
	}
	QProcess proc;
	proc.start("arp -a", QIODevice::Text | QIODevice::ReadOnly);
	proc.waitForFinished();
	//qDebug().noquote()<<QString::fromUtf8(proc.readAllStandardOutput());
	//proc.seek(0);
	auto line = QString::fromUtf8(proc.readLine());
	auto twNetNeighborWidget = findChild<QTreeWidget*>("twNetNeighborWidget");
	twNetNeighborWidget->clear();
	twNetNeighborWidget->addTopLevelItem(new QTreeWidgetItem(QStringList() << "localhost" << tr("本机") << tr("正在获取...")));
	while (line.size())
	{
		QRegularExpression ipRe(R"(\d+.\d+.\d+.\d+)");
		QRegularExpression macRe(R"(([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2}))");
		auto ipMatch = ipRe.match(line);
		auto macMatch = macRe.match(line);
		if (ipMatch.hasMatch() && macMatch.hasMatch())
		{
			auto ip = ipMatch.captured(0);
			auto mac = macMatch.captured(0);
			twNetNeighborWidget->addTopLevelItem(new QTreeWidgetItem(QStringList() << ip << mac << tr("正在获取...")));
		}
		twNetNeighborWidget->setHeaderItem(new QTreeWidgetItem(QStringList() << tr("IP地址") << tr("MAC地址") << ("服务器状态")));
		//output += line;
		line = QString::fromUtf8(proc.readLine());
	}
	{
		// get server status
		auto serverStatus = new int[twNetNeighborWidget->topLevelItemCount()];
		std::fill(serverStatus, serverStatus + twNetNeighborWidget->topLevelItemCount(), 0xabcdef11);
		m_appConfig["D_serverStatus"] = (qint64)serverStatus;
		//qDebug() << "server status result poitner:" << serverStatus;
		m_appConfig["D_serverStatusSize"] = twNetNeighborWidget->topLevelItemCount();
		FindServer fs(this, m_appConfig);
		QTimer().singleShot(m_appConfig["statusTimeout"].toInt() * 1.5, this, &AutoCapture::onCollectServerStatus);
	}


}


void AutoCapture::onDisconnectFromServer()
{
	auto twConnectedServer = findChild<QTreeWidget*>("twConnectedServer");
	auto items = twConnectedServer->selectedItems();
	QTreeWidgetItem* item = nullptr;
	if (items.size())
	{
		item = items[0];
	}
	else if (twConnectedServer->topLevelItemCount() == 1)
	{
		item = twConnectedServer->topLevelItem(0);
	}
	else
	{
		toLog(tr("未选中任何服务器, 无法断开连接."), 1);
		return;
	}
	auto ip = item->text(0);
	bool hostFound = false;
	for (auto socket : m_peerServers)
	{
		if (socket->peerAddress().toString() == ip)
		{
			hostFound = true;
			socket->disconnectFromHost();	// do not modify peerServers because the slot onSocketClosed will do it.
			break;
		}
	}
	if (!hostFound)
	{
		toLog(tr("未找到") + ip + tr("的服务器, 无法断开连接."), 2);
	}
}