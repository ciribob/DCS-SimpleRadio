#include "DCSPosition.h"
#include <cmath>

namespace SimpleRadio
{
	DCSPosition::DCSPosition(float x, float y)
		: x(x)
		, y(y)
	{
	}

	float DCSPosition::distanceTo(const DCSPosition& vector) const
	{
		float dx = this->x - vector.x;
		float dy = this->y - vector.y;
	

		return sqrtf((dx * dx) + (dy * dy));
	}
}