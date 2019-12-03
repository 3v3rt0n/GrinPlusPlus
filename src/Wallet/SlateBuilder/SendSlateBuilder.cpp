#include "SendSlateBuilder.h"
#include "TransactionBuilder.h"
#include "OutputBuilder.h"
#include "CoinSelection.h"

#include <Core/Exceptions/WalletException.h>
#include <Wallet/WalletUtil.h>
#include <Crypto/CryptoUtil.h>
#include <Common/Util/FunctionalUtil.h>
#include <Infrastructure/Logger.h>
#include <Consensus/HardForks.h>
#include <Net/Tor/TorAddressParser.h>
#include <Crypto/ED25519.h>

SendSlateBuilder::SendSlateBuilder(const Config& config, INodeClientConstPtr pNodeClient)
	: m_config(config), m_pNodeClient(pNodeClient)
{

}

Slate SendSlateBuilder::BuildSendSlate(
	Locked<Wallet> wallet, 
	const SecureVector& masterSeed, 
	const uint64_t amount, 
	const uint64_t feeBase,
	const uint8_t numOutputs,
	const std::optional<std::string>& addressOpt,
	const std::optional<std::string>& messageOpt, 
	const SelectionStrategyDTO& strategy,
	const uint16_t slateVersion) const
{
	// Set lock_height for transaction kernel (current chain height).
	const uint64_t blockHeight = m_pNodeClient->GetChainHeight() + 1;
	const uint16_t headerVersion = Consensus::GetHeaderVersion(m_config.GetEnvironment().GetEnvironmentType(), blockHeight);

	// Select inputs using desired selection strategy.
	const uint8_t totalNumOutputs = numOutputs + 1;
	const uint64_t numKernels = 1;
	auto pWallet = wallet.Write();
	const std::vector<OutputData> availableCoins = pWallet->GetAllAvailableCoins(masterSeed);
	std::vector<OutputData> inputs = CoinSelection::SelectCoinsToSpend(
		availableCoins,
		amount,
		feeBase,
		strategy.GetStrategy(),
		strategy.GetInputs(),
		totalNumOutputs,
		numKernels
	);

	// Calculate the fee
	const uint64_t fee = WalletUtil::CalculateFee(feeBase, (int64_t)inputs.size(), totalNumOutputs, numKernels);

	// Create change outputs with total blinding factor xC
	uint64_t inputTotal = 0;
	for (const OutputData& input : inputs)
	{
		inputTotal += input.GetAmount();
	}

	auto pBatch = pWallet->GetDatabase().BatchWrite();
	const uint32_t walletTxId = pBatch->GetNextTransactionId();
	const uint64_t changeAmount = inputTotal - (amount + fee);
	const EBulletproofType bulletproofType = EBulletproofType::ENHANCED;
	const std::vector<OutputData> changeOutputs = OutputBuilder::CreateOutputs(
		pWallet.GetShared(),
		pBatch,
		masterSeed,
		changeAmount,
		walletTxId,
		numOutputs,
		bulletproofType,
		messageOpt
	);

	// Select random transaction offset, and calculate secret key used in kernel signature.
	BlindingFactor transactionOffset = RandomNumberGenerator::GenerateRandom32();
	SecretKey secretKey = CalculatePrivateKey(transactionOffset, inputs, changeOutputs);

	// Select a random nonce kS
	SecretKey secretNonce = Crypto::GenerateSecureNonce();

	// Build Transaction
	Transaction transaction = TransactionBuilder::BuildTransaction(inputs, changeOutputs, transactionOffset, fee, 0);

	// Payment proof
	KeyChainPath torPath = pBatch->GetNextChildPath(KeyChainPath::FromString("m/0/1"));

	std::optional<SlatePaymentProof> proofOpt = std::nullopt;
	auto torAddressOpt = addressOpt.has_value() ? TorAddressParser::Parse(addressOpt.value()) : std::nullopt;
	if (torAddressOpt.has_value())
	{
		SecretKey torKey = KeyChain::FromSeed(m_config, masterSeed).DerivePrivateKey(torPath);
		SecretKey hashedTorKey = Crypto::Blake2b(torKey.GetBytes().GetData());
		ed25519_public_key_t senderAddress = ED25519::CalculatePubKey(hashedTorKey);

		proofOpt = std::make_optional<SlatePaymentProof>(SlatePaymentProof::Create(senderAddress, torAddressOpt.value().GetPublicKey()));
	}

	// Add values to Slate for passing to other participants: UUID, inputs, change_outputs, fee, amount, lock_height, kSG, xSG, oS
	SlateVersionInfo slateVersionInfo(slateVersion, slateVersion, headerVersion);
	Slate slate(
		std::move(slateVersionInfo),
		2,
		uuids::uuid_system_generator()(),
		std::move(transaction),
		amount,
		fee,
		blockHeight,
		0,
		proofOpt
	);
	AddSenderInfo(slate, secretKey, secretNonce, messageOpt);

	WalletTx walletTx = BuildWalletTx(walletTxId, inputs, changeOutputs, slate, addressOpt, messageOpt);

	SlateContext slateContext(
		std::move(secretKey),
		std::move(secretNonce),
		proofOpt.has_value() ? std::make_optional<KeyChainPath>(torPath) : std::nullopt,
		proofOpt.has_value() ? std::make_optional<ed25519_public_key_t>(torAddressOpt.value().GetPublicKey()) : std::nullopt 
	);
	UpdateDatabase(pBatch, masterSeed, slate.GetSlateId(), slateContext, changeOutputs, inputs, walletTx);

	pBatch->Commit();

	return slate;
}

SecretKey SendSlateBuilder::CalculatePrivateKey(
	const BlindingFactor& transactionOffset,
	const std::vector<OutputData>& inputs,
	const std::vector<OutputData>& changeOutputs) const
{
	auto getBlindingFactors = [](OutputData& output) -> BlindingFactor { return BlindingFactor(output.GetBlindingFactor().GetBytes()); };

	// Calculate sum inputs blinding factors xI.
	std::vector<BlindingFactor> inputBlindingFactors = FunctionalUtil::map<std::vector<BlindingFactor>>(inputs, getBlindingFactors);
	BlindingFactor inputBFSum = Crypto::AddBlindingFactors(inputBlindingFactors, std::vector<BlindingFactor>());

	// Calculate sum change outputs blinding factors xC.
	std::vector<BlindingFactor> outputBlindingFactors = FunctionalUtil::map<std::vector<BlindingFactor>>(changeOutputs, getBlindingFactors);
	BlindingFactor outputBFSum = Crypto::AddBlindingFactors(outputBlindingFactors, std::vector<BlindingFactor>());

	// Calculate total blinding excess sum for all inputs and outputs xS1 = xC - xI
	BlindingFactor totalBlindingExcessSum = CryptoUtil::AddBlindingFactors(&outputBFSum, &inputBFSum);

	// Subtract random kernel offset oS from xS1. Calculate xS = xS1 - oS
	BlindingFactor privateKeyBF = CryptoUtil::AddBlindingFactors(&totalBlindingExcessSum, &transactionOffset);

	return SecretKey(CBigInteger<32>(privateKeyBF.GetBytes()));
}

void SendSlateBuilder::AddSenderInfo(
	Slate& slate,
	const SecretKey& secretKey,
	const SecretKey& secretNonce,
	const std::optional<std::string>& messageOpt) const
{
	PublicKey publicKey = Crypto::CalculatePublicKey(secretKey);
	PublicKey publicNonce = Crypto::CalculatePublicKey(secretNonce);

	ParticipantData participantData(0, publicKey, publicNonce);

	// Add message signature
	if (messageOpt.has_value())
	{
		// TODO: Limit message length
		std::unique_ptr<CompactSignature> pMessageSignature = Crypto::SignMessage(secretKey, publicKey, messageOpt.value());
		if (pMessageSignature == nullptr)
		{
			WALLET_ERROR_F("Failed to sign message for slate %s", uuids::to_string(slate.GetSlateId()));
			throw CryptoException();
		}

		participantData.AddMessage(messageOpt.value(), *pMessageSignature);
	}

	slate.AddParticpantData(participantData);
}

WalletTx SendSlateBuilder::BuildWalletTx(
	const uint32_t walletTxId,
	const std::vector<OutputData>& inputs,
	const std::vector<OutputData>& outputs,
	const Slate& slate,
	const std::optional<std::string>& addressOpt,
	const std::optional<std::string>& messageOpt) const
{
	uint64_t amountDebited = 0;
	for (const OutputData& input : inputs)
	{
		amountDebited += input.GetAmount();
	}

	uint64_t amountCredited = 0;
	for (const OutputData& output : outputs)
	{
		amountCredited += output.GetAmount();
	}

	return WalletTx(
		walletTxId,
		EWalletTxType::SENDING_STARTED, // TODO: Change EWalletTxType to EWalletTxStatus
		std::make_optional(slate.GetSlateId()),
		std::optional<std::string>(addressOpt),
		std::optional<std::string>(messageOpt),
		std::chrono::system_clock::now(),
		std::nullopt,
		std::nullopt,
		amountCredited,
		amountDebited,
		std::make_optional(slate.GetFee()),
		std::make_optional(slate.GetTransaction())
	);
}

void SendSlateBuilder::UpdateDatabase(
	Writer<IWalletDB> pBatch,
	const SecureVector& masterSeed,
	const uuids::uuid& slateId,
	const SlateContext& context,
	const std::vector<OutputData>& changeOutputs,
	std::vector<OutputData>& coinsToLock,
	const WalletTx& walletTx) const
{
	// Save secretKey and secretNonce
	pBatch->SaveSlateContext(masterSeed, slateId, context);

	// Save new blinded outputs
	pBatch->AddOutputs(masterSeed, changeOutputs);

	// Lock coins
	for (OutputData& coin : coinsToLock)
	{
		coin.SetStatus(EOutputStatus::LOCKED);
	}

	pBatch->AddOutputs(masterSeed, coinsToLock);

	// Save the log/WalletTx
	pBatch->AddTransaction(masterSeed, walletTx);
}