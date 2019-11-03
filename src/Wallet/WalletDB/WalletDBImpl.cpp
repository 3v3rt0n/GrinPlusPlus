#include <Wallet/WalletDB/WalletDB.h>

#ifndef __linux__ 
#include "WalletRocksDB.h"
#endif

#include "WalletSqlite.h"
#include <Infrastructure/Logger.h>
#include <Wallet/WalletDB/WalletStoreException.h>

namespace WalletDBAPI
{
	//
	// Opens the wallet database and returns an instance of IWalletDB.
	//
	WALLET_DB_API std::shared_ptr<IWalletDB> OpenWalletDB(const Config& config)
	{
#ifdef __linux__
		return WalletSqlite::Open(config);
#else
		std::shared_ptr<WalletSqlite> pWalletDB = WalletSqlite::Open(config);

		if (config.GetWalletConfig().GetDatabaseType() != "SQLITE")
		{
			// The wallet no longer supports the RocksDB database.
			// This logic migrates any existing RocksDB wallets over to SQLITE.
			std::shared_ptr<WalletRocksDB> pRocksDB = WalletRocksDB::Open(config);

			const std::vector<std::string> accounts = pRocksDB->GetAccounts();
			for (const std::string& account : accounts)
			{
				try
				{
					std::unique_ptr<EncryptedSeed> pSeed = pRocksDB->LoadWalletSeed(account);
					if (pSeed != nullptr)
					{
						pWalletDB->CreateWallet(account, *pSeed);
					}
				}
				catch (WalletStoreException&)
				{
					LOG_INFO_F("Error occurred while migrating %s", account);
				}
			}
		}

		return std::shared_ptr<IWalletDB>(pWalletDB);
#endif
	}
}