#pragma once

#include <Net/IPAddress.h>

#include <Core/Traits/Printable.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class SocketAddress : public Traits::IPrintable
{
public:
	//
	// Constructors
	//
	SocketAddress(IPAddress&& ipAddress, const uint16_t port)
		: m_ipAddress(std::move(ipAddress)), m_port(port)
	{

	}

	SocketAddress(const IPAddress& ipAddress, const uint16_t port)
		: m_ipAddress(ipAddress), m_port(port)
	{

	}

	SocketAddress(const std::string& ipAddress, const uint16_t port)
		: m_ipAddress(IPAddress::Parse(ipAddress)), m_port(port)
	{

	}

	SocketAddress(const SocketAddress& other) = default;
	SocketAddress(SocketAddress&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~SocketAddress() = default;

	//
	// Operators
	//
	SocketAddress& operator=(const SocketAddress& other) = default;
	SocketAddress& operator=(SocketAddress&& other) noexcept = default;

	//
	// Getters
	//
	const IPAddress& GetIPAddress() const { return m_ipAddress; }
	const uint16_t GetPortNumber() const { return m_port; }
	virtual std::string Format() const override final
	{
		return m_ipAddress.Format() + ":" + std::to_string(m_port);
	}

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const
	{
		m_ipAddress.Serialize(serializer);

		serializer.Append<uint16_t>(m_port);
	}

	static SocketAddress Deserialize(ByteBuffer& byteBuffer)
	{
		IPAddress ipAddress = IPAddress::Deserialize(byteBuffer);
		const uint16_t port = byteBuffer.ReadU16();

		return SocketAddress(std::move(ipAddress), port);
	}

private:
	IPAddress m_ipAddress;
	uint16_t m_port;
};