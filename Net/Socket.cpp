#include <Net/Socket.h>
#include <Net/SocketException.h>
#include <Common/Util/ThreadUtil.h>
#include <Infrastructure/Logger.h>

static unsigned long DEFAULT_TIMEOUT = 5 * 1000; // 5s

Socket::Socket(const SocketAddress& address)
	: m_address(address), m_socketOpen(false), m_blocking(true), m_receiveBufferSize(0), m_receiveTimeout(DEFAULT_TIMEOUT), m_sendTimeout(DEFAULT_TIMEOUT)
{

}

bool Socket::Connect(asio::io_context& context)
{
	asio::ip::tcp::endpoint endpoint(asio::ip::address(asio::ip::address_v4::from_string(m_address.GetIPAddress().Format())), m_address.GetPortNumber());

	m_pSocket = std::make_shared<asio::ip::tcp::socket>(context);
	m_pSocket->async_connect(endpoint, [this](const asio::error_code & ec)
		{
			m_errorCode = ec;
			if (!ec)
			{
				if (setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, (char*)& DEFAULT_TIMEOUT, sizeof(DEFAULT_TIMEOUT)) == SOCKET_ERROR)
				{
					return false;
				}

				if (setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_SNDTIMEO, (char*)& DEFAULT_TIMEOUT, sizeof(DEFAULT_TIMEOUT)) == SOCKET_ERROR)
				{
					return false;
				}

				m_address = SocketAddress(m_address.GetIPAddress(), m_pSocket->remote_endpoint().port());
				m_socketOpen = true;
				return true;
			}
			else
			{
				asio::error_code ignoreError;
				m_pSocket->close(ignoreError);
				return false;
			}
		}
	);

	context.run();

	auto timeout = std::chrono::system_clock::now() + std::chrono::seconds(1);
	while (!m_errorCode && !m_socketOpen && std::chrono::system_clock::now() < timeout)
	{
		ThreadUtil::SleepFor(std::chrono::milliseconds(10), false);
	}

	return m_socketOpen;
}

bool Socket::Accept(asio::io_context& context, asio::ip::tcp::acceptor& acceptor, const std::atomic_bool& terminate)
{
	m_pSocket = std::make_shared<asio::ip::tcp::socket>(context);
	acceptor.async_accept(*m_pSocket, [this, &context](const asio::error_code & ec)
		{
			m_errorCode = ec;
			if (!ec)
			{
				if (setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, (char*)& DEFAULT_TIMEOUT, sizeof(DEFAULT_TIMEOUT)) == SOCKET_ERROR)
				{
					return false;
				}

				if (setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_SNDTIMEO, (char*)& DEFAULT_TIMEOUT, sizeof(DEFAULT_TIMEOUT)) == SOCKET_ERROR)
				{
					return false;
				}

				const std::string address = m_pSocket->remote_endpoint().address().to_string();
				m_address = SocketAddress(IPAddress::FromString(address), m_pSocket->remote_endpoint().port());
				m_socketOpen = true;
				return true;
			}
			else
			{
				asio::error_code ignoreError;
				m_pSocket->close(ignoreError);
				return false;
			}
		}
	);

	context.run();

	while (!m_errorCode && !m_socketOpen)
	{
		if (terminate)
		{
			acceptor.cancel();
		}

		ThreadUtil::SleepFor(std::chrono::milliseconds(10), false);
	}

	context.reset();

	return m_socketOpen;
}

bool Socket::CloseSocket()
{
	m_socketOpen = false;

	asio::error_code error;
	m_pSocket->shutdown(asio::socket_base::shutdown_both, error);
	if (!error)
	{
		m_pSocket->close(error);
	}

	return !error;
}

bool Socket::IsSocketOpen() const
{
	return m_socketOpen;
}

bool Socket::IsActive() const
{
	if (m_socketOpen && !m_errorCode)
	{
		return true;
	}

	return false;
}

bool Socket::SetReceiveTimeout(const unsigned long milliseconds)
{
	if (m_receiveTimeout != milliseconds)
	{
		const int result = setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, (char*)&milliseconds, sizeof(milliseconds));
		if (result == 0)
		{
			m_receiveTimeout = milliseconds;
		}
	}

	return m_receiveTimeout == milliseconds;
}

bool Socket::SetReceiveBufferSize(const int bufferSize)
{
	if (m_receiveBufferSize != bufferSize)
	{
		const int socketRcvBuff = bufferSize;
		const int result = setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_RCVBUF, (const char*)&socketRcvBuff, sizeof(int));
		if (result == 0)
		{
			m_receiveBufferSize = bufferSize;
		}
	}

	return m_receiveBufferSize == bufferSize;
}

bool Socket::SetSendTimeout(const unsigned long milliseconds)
{
	if (m_sendTimeout != milliseconds)
	{
		const int result = setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_SNDTIMEO, (char*)&milliseconds, sizeof(milliseconds));
		if (result == 0)
		{
			m_sendTimeout = milliseconds;
		}
	}

	return m_sendTimeout == milliseconds;
}

bool Socket::SetBlocking(const bool blocking)
{
	if (m_blocking != blocking)
	{
		// TODO: Just change m_blocking value, and read it when using send/recieve?
		unsigned long blockingValue = (blocking ? 0 : 1);
		const int result = ioctlsocket(m_pSocket->native_handle(), FIONBIO, &blockingValue);
		if (result == 0)
		{
			m_blocking = blocking;
		}
		else
		{
			int error = 0;
			int size = sizeof(error);
			getsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_ERROR, (char*)&error, &size);
			WSASetLastError(error);
			throw SocketException();
		}
	}

	return m_blocking == blocking;
}

bool Socket::Send(const std::vector<unsigned char>& message)
{
	const size_t bytesWritten = asio::write(*m_pSocket, asio::buffer(message.data(), message.size()), m_errorCode);
	if (m_errorCode)
	{
		throw SocketException();
	}

	return bytesWritten == message.size();
}

bool Socket::Receive(const size_t numBytes, std::vector<unsigned char>& data)
{
	if (data.size() < numBytes)
	{
		data = std::vector<unsigned char>(numBytes);
	}

	const size_t bytesRead = asio::read(*m_pSocket, asio::buffer(data.data(), numBytes), m_errorCode);
	if (m_errorCode)
	{
		throw SocketException();
	}

	return bytesRead == numBytes;
}

bool Socket::HasReceivedData()
{
	const size_t available = m_pSocket->available(m_errorCode);
	if (m_errorCode)
	{
		throw SocketException();
	}
	
	return available >= 11;
}