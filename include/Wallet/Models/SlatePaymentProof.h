#pragma once

#include <Crypto/ED25519.h>
#include <Crypto/Signature.h>
#include <Core/Util/JsonUtil.h>
#include <optional>

class SlatePaymentProof
{
public:
	static SlatePaymentProof Create(const ed25519_public_key_t& senderAddress, const ed25519_public_key_t& receiverAddress)
	{
		return SlatePaymentProof(senderAddress, receiverAddress, std::nullopt);
	}

	const ed25519_public_key_t& GetSenderAddress() const { return m_senderAddress; }
	const ed25519_public_key_t& GetReceiverAddress() const { return m_receiverAddress; }
	const std::optional<Signature>& GetReceiverSignature() const { return m_receiverSignatureOpt; }

	Json::Value ToJSON() const
	{
		Json::Value proofJSON;
		proofJSON["sender_address"] = HexUtil::ConvertToHex(m_senderAddress.pubkey);
		proofJSON["receiver_address"] = HexUtil::ConvertToHex(m_receiverAddress.pubkey);
		proofJSON["receiver_signature"] = m_receiverSignatureOpt.has_value() ? m_receiverSignatureOpt.value().ToHex() : Json::Value(Json::nullValue);
		return proofJSON;
	}

	static SlatePaymentProof FromJSON(const Json::Value& paymentProofJSON)
	{
		ed25519_public_key_t senderAddress;
		senderAddress.pubkey = JsonUtil::ConvertToVector(JsonUtil::GetRequiredField(paymentProofJSON, "sender_address"), ED25519_PUBKEY_LEN);

		ed25519_public_key_t receiverAddress;
		receiverAddress.pubkey = JsonUtil::ConvertToVector(JsonUtil::GetRequiredField(paymentProofJSON, "receiver_address"), ED25519_PUBKEY_LEN);

		std::optional<Signature> receiverSignatureOpt = JsonUtil::GetSignatureOpt(paymentProofJSON, "receiver_signature");

		return SlatePaymentProof(senderAddress, receiverAddress, receiverSignatureOpt);
	}

private:
	SlatePaymentProof(
		const ed25519_public_key_t& senderAddress,
		const ed25519_public_key_t& receiverAddress,
		const std::optional<Signature>& receiverSignatureOpt)
		: m_senderAddress(senderAddress), m_receiverAddress(receiverAddress), m_receiverSignatureOpt(receiverSignatureOpt)
	{

	}

	ed25519_public_key_t m_senderAddress;
	ed25519_public_key_t m_receiverAddress;
	std::optional<Signature> m_receiverSignatureOpt;
};