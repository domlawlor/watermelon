#pragma once

#include "defines.h"

#include "LinearMath/btVector3.h"
#include "LinearMath/btQuaternion.h"

static btVector3 Vec3Zero(0.0f, 0.0f, 0.0f);
static btVector3 Vec3Up(0.0f, 1.0f, 0.0f);
static btVector3 Vec3Down(0.0f, -1.0f, 0.0f);
static btVector3 Vec3Forward(0.0f, 0.0f, -1.0f);
static btVector3 Vec3Back(0.0f, 0.0f, 1.0f);
static btVector3 Vec3Left(-1.0f, 0.0f, 0.0f);
static btVector3 Vec3Right(1.0f, 0.0f, 0.0f);

static btQuaternion QuatIdentity(0.0f, 0.0f, 0.0f, 1.0f);

constexpr r32 _PI = 3.14159265358979323846f;
constexpr r32 DegToRad(r32 deg) { return deg * (_PI / 180.0f); }
constexpr r32 RadToDeg(r32 rad) { return rad * (180.0f / _PI); }

struct Transform
{
	btVector3 translation = Vec3Zero;
	btVector3 scale = btVector3(1.0f, 1.0f, 1.0f);
	btQuaternion rotation = QuatIdentity;
};

void SeedRandom(u32 seed);
s32 RandomInt(s32 min, s32 max);
u32 RandomUInt(u32 min, u32 max);
r32 RandomFloat(r32 min, r32 max);
