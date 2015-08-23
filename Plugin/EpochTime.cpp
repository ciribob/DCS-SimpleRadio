#include "EpochTime.h"

double seconds()
{
	SYSTEMTIME t;
	GetSystemTime(&t);
	return (
		
		(double)t.wDay * 3.156e+7l
		+(double)t.wMonth * 2.63e+6l
		+ (double)t.wDay * 86400
		+ (double)t.wHour * 3600
		+ (double)t.wMinute * 60
		+ (double)t.wSecond
		+ (double)t.wMilliseconds * 0.01);
}