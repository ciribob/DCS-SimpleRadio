#include "RegHelper.h"



RegHelper::RegHelper()
{
}


RegHelper::~RegHelper()
{
}

std::string RegHelper::readSRPath()
{


	HKEY hKey = 0;
	DWORD bufferSize = 512;
	LPVOID regBuffer = new char[bufferSize];
	DWORD dwType = 0;
	if (RegOpenKey(HKEY_CURRENT_USER, L"SOFTWARE\\DCS-SimpleRadio", &hKey) == ERROR_SUCCESS)
	{
		dwType = REG_SZ;
		if (RegQueryValueEx(hKey, L"SRPath", 0, &dwType, (BYTE*)regBuffer, &bufferSize) == ERROR_SUCCESS)
		{
			
			//its a 2 Byte CHAR! 
			WCHAR *locationStr = reinterpret_cast<WCHAR *>(regBuffer);
			locationStr[bufferSize] = 0; //add terminator

										 //convert to widestring
			std::wstring ws(locationStr);
			//convert to normal string
			std::string str(ws.begin(), ws.end());


			RegCloseKey(hKey);

			delete[] regBuffer;
			return str;
		}
		
		RegCloseKey(hKey);
	}

	delete[] regBuffer;
	
	return "";

}
