#include "OwnerReceive.h"

#include <Net/Tor/TorManager.h>
#include <Net/Tor/TorAddressParser.h>
#include <Common/Util/FileUtil.h>

OwnerReceive::OwnerReceive(IWalletManagerPtr pWalletManager)
	: m_pWalletManager(pWalletManager)
{

}

RPC::Response OwnerReceive::Handle(const RPC::Request& request) const
{
	if (!request.GetParams().has_value())
	{
		throw DESERIALIZATION_EXCEPTION();
	}

	ReceiveCriteria criteria = ReceiveCriteria::FromJSON(request.GetParams().value());

	if (criteria.GetFile().has_value())
	{
		return ReceiveViaFile(request, criteria, criteria.GetFile().value());
	}
	else
	{
		Slate slate = m_pWalletManager->Receive(criteria);

		Json::Value result;
		result["status"] = "RECEIVED";
		result["slate"] = slate.ToJSON();
		return request.BuildResult(result);
	}
}

RPC::Response OwnerReceive::ReceiveViaFile(const RPC::Request& request, const ReceiveCriteria& criteria, const std::string& file) const
{
	// FUTURE: Check write permissions before creating slate

	Slate slate = m_pWalletManager->Receive(criteria);

	Json::Value slateJSON = slate.ToJSON();
	if (FileUtil::WriteTextToFile(file, JsonUtil::WriteCondensed(slateJSON)))
	{
		Json::Value result;
		result["status"] = "RECEIVED";
		result["slate"] = slateJSON;
		return RPC::Response::BuildResult(request.GetId(), result);
	}
	else
	{
		Json::Value errorJson;
		errorJson["status"] = "WRITE_FAILED";
		errorJson["slate"] = slateJSON;
		return request.BuildError(RPC::ErrorCode::INTERNAL_ERROR, "Failed to write file.", errorJson);
	}
}