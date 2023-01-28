#pragma once

#include "defines.h"

class PhysicsWorld;

struct ShipInput
{
	enum Action
	{
		THRUST_X,
		THRUST_Y,
		THRUST_Z,
		PITCH, // X Axis
		YAW, // Y Axis
		ROLL, // Z Axis

		FIRE,
		ALT_FIRE,

		ACTION_TOTAL
	};
	r32 inputs[ACTION_TOTAL] = {};
};

struct ShipConfig
{
	r32 thrustSpeed = 20.0f;
	r32 pitchRate = 5.0f;
	r32 yawRate = 5.0f;
	r32 rollRate = 30.0f;
};

class Game
{
public:
	Game(u32 windowWidth, u32 windowHeight);
	~Game();

	void UpdateAndDraw();

private:
	void Load();

	entt::registry m_registry;

	std::vector<entt::entity> m_entities;
	entt::entity m_playerEntity = entt::null;

	PhysicsWorld *m_physics;

	r64 m_lastFrameTime = 0;

	u32 m_windowWidth = 0;
	u32 m_windowHeight = 0;


	struct DebugFlags
	{
		bool drawCollision = false;
	};
	DebugFlags debugFlags;
};