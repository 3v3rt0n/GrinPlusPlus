#pragma once

#include <Common/Secure.h>
#include <Crypto/Commitment.h>
#include <Wallet/WalletDB/Models/OutputDataEntity.h>
#include <Wallet/Models/DTOs/WalletTxDTO.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <Wallet/WalletTx.h>
#include <unordered_set>
#include <vector>

class CancelTx
{
public:
	static void CancelWalletTx(const SecureVector& masterSeed, Locked<IWalletDB> walletDB, WalletTx& walletTx);

private:
	static std::vector<OutputDataEntity> GetOutputsToUpdate(
		std::vector<OutputDataEntity>& outputs,
		const std::unordered_set<Commitment>& inputCommitments,
		const WalletTx& walletTx
	);
};