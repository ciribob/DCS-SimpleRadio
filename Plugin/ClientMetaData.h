#ifndef SR_CLIENTMETADATA_H
#define SR_CLIENTMETADATA_H

#include <string>
#include "RadioInformation.h"

namespace SimpleRadio
{
	class ClientMetaData
	{
	public:
		ClientMetaData();
		std::string serialize(bool formatted = false) const;
		static const ClientMetaData deserialize(const std::string& document, bool fromUDP);
		bool isCurrent();

		bool isEqual(ClientMetaData &data);

		unsigned long long lastUpdate;
		std::string name;
		std::string unit;
		int selected;
		RadioInformation radio[3];
		bool hasRadio;
		bool groundCommander;
		int unitId;
	};
};

#endif
