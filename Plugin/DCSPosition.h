#ifndef SR_VECTOR3_H
#define SR_VECTOR3_H

namespace SimpleRadio
{
	class DCSPosition
	{
	public:
		DCSPosition(float x = 0.0f, float y = 0.0f);
		float distanceTo(const DCSPosition& vector) const;
		float x;
		float y;
	};
};

#endif