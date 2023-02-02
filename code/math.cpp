#include "math.h"


void SeedRandom(u32 seed)
{
	srand(seed);
}

s32 RandomInt(s32 min, s32 max)
{
	if(min == max) return 0;
	Assert(min < max);
	Assert(max <= RAND_MAX);
	s32 range = max - min;
	s32 result = (rand() % range) + min;
	return result;
}

u32 RandomUInt(u32 min, u32 max)
{
	if(min == max) return 0;
	Assert(min < max);
	Assert(max <= RAND_MAX);
	u32 range = max - min;
	u32 result = (rand() % range) + min;
	return result;
}

r32 RandomFloat(r32 min, r32 max)
{
	Assert(min < max);
	r32 zeroToOneRand = (r32)(rand()) / (r32)(RAND_MAX);
	r32 result = ((max - min) * zeroToOneRand) + min;
	return result;
}
