#pragma once
#include <string>
struct RadioInformation
{
	std::string name;
	double frequency;
	int modulation;
	float volume;
	double secondaryFrequency;
	double freqMin;
	double freqMax;
};