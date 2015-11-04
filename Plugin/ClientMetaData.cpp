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
		, hasRadio(true)
		, groundCommander(false)
		, unitId(-1)
	{
		for (int i = 0; i < 3; i++)
		{
			this->radio[i].name = "No Radio";
			this->radio[i].frequency = -1;
			this->radio[i].modulation = 0;
			this->radio[i].volume = 1.0;
			this->radio[i].secondaryFrequency = -1;
		}
	}

	string ClientMetaData::serialize(bool formatted) const
	{
		Json::Value root;
		root["lastUpdate"] = this->lastUpdate;
		root["name"] = this->name;
		root["unit"] = this->unit;
		root["selected"] = this->selected;
		root["unitId"] = this->unitId;

		Json::Value array;
		for (int i = 0; i < 3; i++)
		{
			Json::Value current;
			current["name"] = this->radio[i].name;
			current["frequency"] = this->radio[i].frequency;
			current["modulation"] = this->radio[i].modulation;
			current["volume"] = this->radio[i].volume;
			current["secondaryFrequency"] = this->radio[i].secondaryFrequency;
			array.append(current);
		}

		root["radios"] = array;

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
			data.selected = root["selected"].asInt();

			for (int i = 0; i < 3; i++)
			{
				data.radio[i].name = root["radios"][i]["name"].asString();
				data.radio[i].frequency = std::stod(root["radios"][i]["frequency"].asString());
				data.radio[i].modulation = root["radios"][i]["modulation"].asInt();
				data.radio[i].volume = root["radios"][i]["volume"].asFloat();

				try {
					data.radio[i].secondaryFrequency = std::stod(root["radios"][i]["secondaryFrequency"].asString());
				}
				catch (...)
				{
					//catch older versions
					data.radio[i].secondaryFrequency = -1;
				}

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

			try {

				data.unitId = root["unitId"].asInt();
			}
			catch (...)
			{
				//catch older versions
				data.unitId = -1;
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

	bool ClientMetaData::isEqual(ClientMetaData &data)
	{

		for (int i = 0; i < 3; i++)
		{
			if (this->radio[i].name.compare(data.radio[i].name) != 0)
			{
				return false;
			}
			if (this->radio[i].frequency != data.radio[i].frequency)
			{
				return false;
			}
			if (this->radio[i].modulation != data.radio[i].modulation)
			{
				return false;
			}
			if (this->radio[i].volume != data.radio[i].volume)
			{
				return false;
			}

		}

		if (this->groundCommander != data.groundCommander)
		{
			return false;
		}
		if (this->hasRadio != data.hasRadio)
		{
			return false;
		}
		if (this->name.compare(data.name) != 0)
		{
			return false;
		}
		if (this->unit.compare(data.unit) != 0)
		{
			return false;
		}
		if (this->selected != data.selected)
		{
			return false;
		}
		if (this->unitId != data.unitId)
		{
			return false;
		}

		return true;

	}

}