#pragma once

#include <Wallet/Enums/SelectionStrategy.h>
#include <Crypto/Commitment.h>
#include <Core/Util/JsonUtil.h>
#include <set>

class SelectionStrategyDTO
{
public:
	SelectionStrategyDTO(const ESelectionStrategy& strategy, std::set<Commitment>&& inputs)
		: m_strategy(strategy), m_inputs(std::move(inputs))
	{

	}

	static SelectionStrategyDTO FromJSON(const Json::Value& selectionStrategyJSON)
	{
		const std::string selectionStrategy = JsonUtil::GetRequiredString(selectionStrategyJSON, "strategy");

		std::set<Commitment> inputs;
		const std::optional<Json::Value> inputsJSON = JsonUtil::GetOptionalField(selectionStrategyJSON, "inputs");
		if (inputsJSON.has_value())
		{
			for (Json::Value::const_iterator iter = inputsJSON.value().begin(); iter != inputsJSON.value().end(); iter++)
			{
				inputs.insert(JsonUtil::ConvertToCommitment(*iter));
			}
		}

		return SelectionStrategyDTO(SelectionStrategy::FromString(selectionStrategy), std::move(inputs));
	}

	ESelectionStrategy GetStrategy() const { return m_strategy; }
	const std::set<Commitment>& GetInputs() const { return m_inputs; }

private:
	ESelectionStrategy m_strategy;
	std::set<Commitment> m_inputs;
};