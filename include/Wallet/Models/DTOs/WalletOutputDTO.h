#pragma once

#include <Wallet/WalletDB/Models/OutputDataEntity.h>
#include <Core/Util/JsonUtil.h>

class WalletOutputDTO
{
public:
	WalletOutputDTO(const OutputDataEntity& outputData) : m_outputData(outputData) { }
	WalletOutputDTO(OutputDataEntity&& outputData) : m_outputData(std::move(outputData)) { }

	Json::Value ToJSON() const
	{
		Json::Value outputJSON;

		outputJSON["keychain_path"] = m_outputData.GetKeyChainPath().Format();
		outputJSON["commitment"] = m_outputData.GetOutput().GetCommitment().ToHex();
		outputJSON["amount"] = m_outputData.GetAmount();
		outputJSON["status"] = OutputStatus::ToString(m_outputData.GetStatus());

		JsonUtil::AddOptionalField(outputJSON, "mmr_index", m_outputData.GetMMRIndex());
		JsonUtil::AddOptionalField(outputJSON, "block_height", m_outputData.GetBlockHeight());
		JsonUtil::AddOptionalField(outputJSON, "transaction_id", m_outputData.GetWalletTxId());
		JsonUtil::AddOptionalField(outputJSON, "label", m_outputData.GetLabel());

		return outputJSON;
	}

private:
	OutputDataEntity m_outputData;
};