#pragma once

#include <iostream>
#include <string>
#include <Windows.h>



class RegHelper
{
public:
	RegHelper();
	~RegHelper();

	std::string readSRPath();

};

