#ifndef SR_CONFIGURATION_H
#define SR_CONFIGURATION_H

#include <string>

namespace SimpleRadio
{
	class Configuration
	{
	public:
		Configuration();
		virtual ~Configuration();
		static Configuration load();
		const std::wstring& getSelectPttOneDevice() const;
		int getSelectPttOneButton() const;
		const std::wstring& getSelectPttTwoDevice() const;
		int getSelectPttTwoButton() const;
		const std::wstring& getSelectPttThreeDevice() const;
		int getSelectPttThreeButton() const;
		const std::wstring& getPttCommonDevice() const;
		int getPttCommonButton() const;
		float getRadioOnePan() const;
		float getRadioTwoPan() const;
		float getRadioThreePan() const;

	private:
		static std::wstring getIniFilePath();
		std::wstring selectPttOneDevice;
		int selectPttOneButton;
		std::wstring selectPttTwoDevice;
		int selectPttTwoButton;
		std::wstring selectPttThreeDevice;
		int selectPttThreeButton;
		std::wstring pttCommonDevice;
		int pttCommonButton;
		float radioOnePan;
		float radioTwoPan;
		float radioThreePan;
	};
};

#endif
