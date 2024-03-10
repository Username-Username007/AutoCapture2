#pragma once
#include <qbytearray.h>
#include <qiodevice.h>
class DataPacket
{
protected:
	QByteArray m_data;
	std::size_t m_expectedSize = 0;		// the size that has been read from a stream.
public:
	enum  DataType : std::int32_t
	{
		Invalid = (1<<31),	// the packet is incomplete or invalid (can be used as a flag)
		Text = 0,	// to be shown in the log window
		TestVideoFrame,	// for testing video display, not supposed to be used for application
	} ;
	static_assert(sizeof(DataType) == 4, "DataType must be 4 bytes long.");
	std::int32_t m_dataType;
public:

	void clear()
	{
		m_dataType = DataType::Invalid;
		m_data.clear();
	}
	auto getExpectedSize()const
	{
		return m_expectedSize;
	}
	// parameter: Ensure the begining of the stream is the begining of the packet
	// returns true if the packet is complete, false if the packet is incomplete
	// packet structure: | packet size (8 bytes) | data type (4 bytes) | data part |
	bool appendFromStream(QIODevice* stream, const int timeout = 100)
	{
		if (m_dataType == Invalid)	// only the empty packet is invalid, other packets are Invalid | DATATYPE
		{
			//qDebug() << "DataPacket: Begining of a new packet";
			// begining of a new packet
			auto sizeByte = stream->read(8);
			while (sizeByte.size() < 8)
			{
				sizeByte += stream->read(8 - sizeByte.size());
				stream->waitForReadyRead(timeout);
			}
			auto typeByte = stream->read(4);
			while (typeByte.size() < 4)
			{
				typeByte += stream->read(4 - typeByte.size());
				stream->waitForReadyRead(timeout);
			}
			m_expectedSize = *(std::int64_t*)sizeByte.constData();
			//qDebug() << "DataPacket: expected size:" << m_expectedSize;
			m_dataType = *(std::int32_t*)typeByte.constData();
			//qDebug() << "DataPacket: data type:" << m_dataType;

		}
		auto dataToWrite = stream->read(m_expectedSize-4-m_data.size());
		m_data.append(dataToWrite);
#ifdef _DEBUG
		qDebug() << "DataPacket: m_data.size()=" << m_data.size();
#endif

		if (m_data.size() == m_expectedSize-4)
		{
			//qDebug() << "DataPacket: packet complete!";
			m_dataType = (m_dataType& ~(DataType::Invalid));
			return true;
		}
		else if (m_data.size() > m_expectedSize - 4)
		{
			QMessageBox(QMessageBox::Icon::Critical, "Error", "DataPacket: The received packet size is larger than expected!. The program will exit!").exec();
			std::exit(-1);
		}
		else
		{
			return false;
		}
	
	}

	auto size() const
	{
		return m_data.size();
	}
	QByteArray generateTcpPacket(const QByteArray& header = "\xff\x0\x0\xff") const
	{
		QByteArray packet;
		packet += header;
		std::int64_t dataSize = m_data.size() + sizeof(DataType);
		packet += QByteArray((char*)&dataSize, sizeof(dataSize));
		packet += QByteArray((char*)&m_dataType, sizeof(m_dataType));
		packet += m_data;

		return packet; // RVO
	}

	DataPacket()
	{
		m_dataType = DataType::Invalid;
		m_expectedSize = 0;
	}
	DataPacket(DataPacket && data) noexcept
	{
		m_data = std::move(data.m_data);
		m_dataType = data.m_dataType;
	}
	DataPacket(DataPacket& data)
	{
		m_data = data.m_data;
		m_dataType = data.m_dataType;
	}

	QByteArray& getData() noexcept
	{
		return m_data;
	}
	DataType getDataType() const
	{
		return (DataType)m_dataType;
	}

	void setData(QByteArray& data, std::int32_t dataType)
	{
		m_dataType = dataType;
		m_data = data;
	}
	void setData(QByteArray&& data, std::int32_t dataType) noexcept
	{
		m_data = std::move(data);
		m_dataType = dataType;
	}

};



