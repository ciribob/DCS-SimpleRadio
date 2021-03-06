#include "json/json.h"

#include "RadioUpdate.h"
#include <windows.h>


using std::string;

namespace SimpleRadio
{
	RadioUpdate::RadioUpdate()
		:name("None")
		, unit("None")
		, selected(0)
		, hasRadio(true)
		, caMode(false)
		, unitId(-1)
	{
		for (int i = 0; i < 3; i++)
		{
			this->radios[i].name = "No Radio";
			this->radios[i].frequency = -1;
			this->radios[i].modulation = 0;
			this->radios[i].volume = 1.0;
			this->radios[i].secondaryFrequency = -1;
		}
	}

	string RadioUpdate::serialize() const
	{
		Json::Value root;

		root["name"] = this->name;
		root["unit"] = this->unit;
		root["selected"] = this->selected;
		root["hasRadio"] = this->hasRadio;
		root["allowNonPlayers"] = this->allowNonPlayers; //if false, non players are muted
		root["caMode"] = this->caMode;
		root["unitId"] = this->unitId;

		Json::Value array;
		for (int i = 0; i < 3; i++)
		{
			Json::Value current;
			current["name"] = this->radios[i].name;
			current["frequency"] = this->radios[i].frequency;
			current["modulation"] = this->radios[i].modulation;
			current["volume"] = this->radios[i].volume;
			current["secondaryFrequency"] = this->radios[i].secondaryFrequency;
			array.append(current);
		}

		root["radios"] = array;
	
		Json::FastWriter writer;
		return writer.write(root);
		
	}




}