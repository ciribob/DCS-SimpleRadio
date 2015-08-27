#ifndef SR_RADIOUPDATE_H
#define SR_RADIOUPDATE_H

#include <string>
#include "RadioInformation.h"
namespace SimpleRadio
{


	class RadioUpdate
	{
	public:
		RadioUpdate();
		std::string serialize(bool formatted = false) const;
		
		std::string name;
		std::string unit;
		int selected;
		
		RadioInformation radios[3];
		bool hasRadio;
		bool allowNonPlayers;
	};
};

#endif
