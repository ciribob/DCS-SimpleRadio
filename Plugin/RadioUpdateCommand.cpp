#include "RadioUpdateCommand.h"
#include "json/json.h"

namespace SimpleRadio
{

	RadioUpdateCommand::RadioUpdateCommand()
	{
	}


	RadioUpdateCommand::~RadioUpdateCommand()
	{
	}

	const RadioUpdateCommand RadioUpdateCommand::deserialize(const std::string& document)
	{
		RadioUpdateCommand data;
		Json::Reader reader;
		Json::Value root;

		bool success = reader.parse(document, root, false);
		if (success == true)
		{
			data.freq = root["freq"].asDouble();
			data.radio = root["radio"].asInt();
			data.volume = root["volume"].asFloat();
			data.cmdType = root["cmdType"].asInt();
		}
		else
		{
			throw std::string("Failed to parse Update Command");
		}

		return data;
	}
}