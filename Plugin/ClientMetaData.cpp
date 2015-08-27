#include "ClientMetaData.h"
#include "json/json.h"

#include <windows.h>


using std::string;

namespace SimpleRadio
{
	ClientMetaData::ClientMetaData()
		: lastUpdate(1)
		, name("init")
		, unit("init")
		, selected(0)
		, position()
		, hasRadio(true)
		, groundCommander(false)
	{
		for (int i = 0; i < 3; i++)
		{
			this->radio[i].name = "No Radio";
			this->radio[i].frequency = -1;
			this->radio[i].modulation = 0;
		}
	}

	string ClientMetaData::serialize(bool formatted) const
	{
		Json::Value root;
		root["lastUpdate"] = this->lastUpdate;
		root["name"] = this->name;
		root["unit"] = this->unit;
		root["selected"] = this->selected;

		Json::Value array;
		for (int i = 0; i < 3; i++)
		{
			Json::Value current;
			current["name"] = this->radio[i].name;
			current["frequency"] = this->radio[i].frequency;
			current["modulation"] = this->radio[i].modulation;
			array.append(current);
		}

		root["radios"] = array;

		Json::Value position;
		position["x"] = this->position.x;
		position["y"] = this->position.y;

		root["pos"] = position;

		root["hasRadio"] = this->hasRadio;

		root["groundCommander"] = this->groundCommander;

		if (formatted == true)
		{
			Json::StyledWriter writer;
			return writer.write(root);
		}
		else
		{
			Json::FastWriter writer;
			return writer.write(root);
		}
	}

	const ClientMetaData ClientMetaData::deserialize(const string& document, bool fromUDP)
	{
		ClientMetaData data;
		Json::Reader reader;
		Json::Value root;
		
		bool success = reader.parse(document, root, false);
		if (success == true)
		{
			data.lastUpdate = GetTickCount64();
			
			data.name = root["name"].asString();
			data.unit = root["unit"].asString();
			//data.unit =  "TODO";
			data.selected = root["selected"].asInt();

			data.position.x = root["pos"]["x"].asFloat();
			data.position.y = root["pos"]["y"].asFloat();

			for (int i = 0; i < 3; i++)
			{
				data.radio[i].name = root["radios"][i]["name"].asString();
				data.radio[i].frequency = std::stod(root["radios"][i]["frequency"].asString());
				data.radio[i].modulation = root["radios"][i]["modulation"].asInt();
			}


			try {
				data.hasRadio = root["hasRadio"].asBool();
			}
			catch (...)
			{
				//catch older versions
				data.hasRadio = true;
			}

			
			try {
				
				data.groundCommander = root["groundCommander"].asBool();
			}
			catch (...)
			{
				//catch older versions
				data.groundCommander = false;
			}
			
			
		}
		else
		{
			throw string("Failed to parse JSON");
		}

		return data;
	}

	/*
		Is Current if we've had an update within the last 5 seconds
	*/
	bool ClientMetaData::isCurrent()
	{
		return this->lastUpdate > (GetTickCount64() - 5000ULL);
	}

}