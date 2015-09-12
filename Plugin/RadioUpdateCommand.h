#pragma once
#include <string>
namespace SimpleRadio
{
	class RadioUpdateCommand
	{
	public:

		double freq;
		int radio;
		float volume;
		int cmdType;

		RadioUpdateCommand();
		~RadioUpdateCommand();

		static const RadioUpdateCommand deserialize(const std::string& document);
	};
}
