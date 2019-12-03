#pragma once

#include <Common/Util/HexUtil.h>
#include <Crypto/BlindingFactor.h>
#include <Crypto/Commitment.h>
#include <Crypto/PublicKey.h>
#include <Crypto/Signature.h>
#include <Crypto/RangeProof.h>
#include <Core/Exceptions/DeserializationException.h>
#include <json/json.h>
#include <optional>
#include <vector>

class JsonUtil
{
public:
	static Json::Value ConvertToJSON(const std::vector<unsigned char>& bytes)
	{
		return Json::Value(HexUtil::ConvertToHex(bytes));
	}

	static std::vector<unsigned char> ConvertToVector(const Json::Value& node, const size_t expectedSize)
	{
		if (node == Json::nullValue)
		{
			throw DESERIALIZATION_EXCEPTION();
		}

		std::vector<unsigned char> bytes = HexUtil::FromHex(std::string(node.asString()));
		if (bytes.size() != expectedSize)
		{
			throw DESERIALIZATION_EXCEPTION();
		}

		return bytes;
	}

	static Json::Value GetRequiredField(const Json::Value& node, const std::string& key)
	{
		if (node == Json::nullValue)
		{
			throw DESERIALIZATION_EXCEPTION();
		}

		const Json::Value value = node.get(key, Json::nullValue);
		if (value == Json::nullValue)
		{
			throw DESERIALIZATION_EXCEPTION();
		}

		return value;
	}

	static std::optional<Json::Value> GetOptionalField(const Json::Value& node, const std::string& key)
	{
		if (node == Json::nullValue)
		{
			throw DESERIALIZATION_EXCEPTION();
		}

		Json::Value value = node.get(key, Json::nullValue);
		if (!value.isNull())
		{
			return std::make_optional(std::move(value));
		}
		else
		{
			return std::nullopt;
		}
	}

	//
	// BlindingFactors
	//
	static Json::Value ConvertToJSON(const BlindingFactor& blindingFactor)
	{
		return ConvertToJSON(blindingFactor.GetBytes().GetData());
	}

	static BlindingFactor ConvertToBlindingFactor(const Json::Value& blindingFactorJSON)
	{
		return BlindingFactor(CBigInteger<32>(ConvertToVector(blindingFactorJSON, 32)));
	}

	static BlindingFactor GetBlindingFactor(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToBlindingFactor(GetRequiredField(parentJSON, key));
	}

	//
	// Commitments
	//
	static Json::Value ConvertToJSON(const Commitment& commitment)
	{
		return ConvertToJSON(commitment.GetBytes().GetData());
	}

	static Commitment ConvertToCommitment(const Json::Value& commitmentJSON)
	{
		return Commitment(CBigInteger<33>(ConvertToVector(commitmentJSON, 33)));
	}

	static Commitment GetCommitment(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToCommitment(GetRequiredField(parentJSON, key));
	}

	//
	// PublicKeys
	//
	static Json::Value ConvertToJSON(const PublicKey& publicKey)
	{
		return ConvertToJSON(publicKey.GetCompressedBytes().GetData());
	}

	static PublicKey ConvertToPublicKey(const Json::Value& publicKeyJSON)
	{
		return PublicKey(CBigInteger<33>(ConvertToVector(publicKeyJSON, 33)));
	}

	static PublicKey GetPublicKey(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToPublicKey(GetRequiredField(parentJSON, key));
	}

	//
	// Signatures
	//
	static Json::Value ConvertToJSON(const Signature& signature)
	{
		return ConvertToJSON(signature.GetSignatureBytes().GetData());
	}

	static Signature ConvertToSignature(const Json::Value& signatureJSON)
	{
		return Signature(CBigInteger<64>(ConvertToVector(signatureJSON, 64)));
	}

	static Signature GetSignature(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToSignature(GetRequiredField(parentJSON, key));
	}

	static Json::Value ConvertToJSON(const std::optional<Signature>& signatureOpt)
	{
		return signatureOpt.has_value() ? ConvertToJSON(signatureOpt.value()) : Json::nullValue;
	}

	static std::optional<CompactSignature> ConvertToSignatureOpt(const Json::Value& signatureJSON)
	{
		if (signatureJSON == Json::nullValue)
		{
			return std::nullopt;
		}

		return std::make_optional(CBigInteger<64>(ConvertToVector(signatureJSON, 64)));
	}

	static std::optional<CompactSignature> GetSignatureOpt(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToSignatureOpt(GetOptionalField(parentJSON, key).value_or(Json::Value(Json::nullValue)));
	}

	//
	// RangeProofs
	//
	static Json::Value ConvertToJSON(const RangeProof& rangeProof)
	{
		return ConvertToJSON(rangeProof.GetProofBytes());
	}

	static RangeProof ConvertToRangeProof(const Json::Value& rangeProofJSON)
	{
		return RangeProof(ConvertToVector(rangeProofJSON, MAX_PROOF_SIZE));
	}

	static RangeProof GetRangeProof(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToRangeProof(GetRequiredField(parentJSON, key));
	}

	//
	// Strings
	//
	static Json::Value ConvertToJSON(const std::optional<std::string>& stringOpt)
	{
		return stringOpt.has_value() ? Json::Value(stringOpt.value()) : Json::nullValue;
	}

	static std::optional<std::string> ConvertToStringOpt(const Json::Value& stringJSON)
	{
		return stringJSON == Json::nullValue ? std::nullopt : std::make_optional(stringJSON.asString());
	}

	static std::optional<std::string> GetStringOpt(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToStringOpt(GetOptionalField(parentJSON, key).value_or(Json::Value(Json::nullValue)));
	}

	static std::string GetRequiredString(const Json::Value& parentJSON, const std::string& key)
	{
		return std::string(GetRequiredField(parentJSON, key).asCString());
	}

	//
	// UInt64
	//
	static uint64_t ConvertToUInt64(const Json::Value& uint64JSON)
	{
		if (uint64JSON.isUInt64())
		{
			return uint64JSON.asUInt64();
		}
		else if (uint64JSON.isString())
		{
			return std::stoull(uint64JSON.asString());
		}
		else
		{
			throw DESERIALIZATION_EXCEPTION();
		}
	}

	static uint64_t GetRequiredUInt64(const Json::Value& parentJSON, const std::string& key)
	{
		const Json::Value value = JsonUtil::GetRequiredField(parentJSON, key);
		return ConvertToUInt64(value);
	}

	static std::optional<uint64_t> GetUInt64Opt(const Json::Value& parentJSON, const std::string& key)
	{
		const std::optional<Json::Value> json = JsonUtil::GetOptionalField(parentJSON, key);
		if (!json.has_value())
		{
			return std::nullopt;
		}
		else
		{
			return ConvertToUInt64(json.value());
		}
	}

	//
	// Optional
	//
	template<class T>
	static void AddOptionalField(Json::Value& json, const std::string& fieldName, const std::optional<T>& opt)
	{
		if (opt.has_value())
		{
			json[fieldName] = opt.value();
		}
	}

	// Parse
	static bool Parse(const std::string& input, Json::Value& outputJSON)
	{
		std::string errors;
		std::istringstream inputStream(input);
		return Json::parseFromStream(Json::CharReaderBuilder(), inputStream, &outputJSON, &errors);
	}

	// Write
	static std::string WriteCondensed(const Json::Value& json)
	{
		Json::StreamWriterBuilder builder;
		builder["indentation"] = ""; // Removes whitespaces
		return Json::writeString(builder, json);
	}
};