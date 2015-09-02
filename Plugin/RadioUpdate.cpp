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
	{
		for (int i = 0; i < 3; i++)
		{
			this->radios[i].name = "No Radio";
			this->radios[i].frequency = -1;
			this->radios[i].modulation = 0;
			this->radios[i].volume = 1.0;
		}
	}

	string RadioUpdate::serialize(bool formatted) const
	{
		Json::Value root;

		root["name"] = this->name;
		root["unit"] = this->unit;
		root["selected"] = this->selected;
		root["hasRadio"] = this->hasRadio;
		root["allowNonPlayers"] = this->allowNonPlayers; //if false, non players are muted

		Json::Value array;
		for (int i = 0; i < 3; i++)
		{
			Json::Value current;
			current["name"] = this->radios[i].name;
			current["frequency"] = this->radios[i].frequency;
			current["modulation"] = this->radios[i].modulation;
			current["volume"] = this->radios[i].volume;
			array.append(current);
		}

		root["radios"] = array;

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




}