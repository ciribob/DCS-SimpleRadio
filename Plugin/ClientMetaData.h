#ifndef SR_CLIENTMETADATA_H
#define SR_CLIENTMETADATA_H

#include <string>
#include "DCSPosition.h"
#include "RadioInformation.h"

namespace SimpleRadio
{
	 struct RadioInformation
	{
		std::string name;
		double frequency;
		int modulation;
		int volume;
	};

	class ClientMetaData
	{
	public:
		ClientMetaData();
		std::string serialize(bool formatted = false) const;
		static const ClientMetaData deserialize(const std::string& document, bool fromUDP);
		bool isCurrent();

		unsigned long long lastUpdate;
		std::string name;
		std::string unit;
		int selected;
		DCSPosition position;
		RadioInformation radio[3];
		bool hasRadio;
		bool groundCommander;
	};
};

#endif
