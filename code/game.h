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


	void SpawnLaserbeam(void *data);

	void AddEvent();

	entt::registry & GetRegistry() { return m_registry; }
	PhysicsWorld * const GetPhysics() { return m_physics; };

	void SetCameraEntity(entt::entity entity) { m_cameraEntity = entity; }

private:
	//void Load();

	entt::registry m_registry;

	std::vector<entt::entity> m_entities;

	entt::entity m_cameraEntity = entt::null;

	PhysicsWorld *m_physics;

	r64 m_lastFrameTime = 0;

	u32 m_windowWidth = 0;
	u32 m_windowHeight = 0;

	
	void *m_eventDataMemory = nullptr;
	u32 m_eventDataMemoryUsed = 0;
	u32 m_eventDataMemorySize = 0;

	using EventDelegate = entt::delegate<void(void *)>;
	struct Event
	{
		EventDelegate delegate;
		void *data = nullptr; 
	};
	std::vector<Event> m_events;

	template<typename T>
	T *AllocEventData(Event &newEvent)
	{
		u32 dataSize = sizeof(T);
		Assert((m_eventDataMemoryUsed + dataSize) <= m_eventDataMemorySize);
		newEvent.data = (void *)((u8 *)m_eventDataMemory + m_eventDataMemoryUsed);
		m_eventDataMemoryUsed += dataSize;
		return (T *)newEvent.data;
	}
	
	

	


	struct DebugFlags
	{
		bool drawCollision = false;
	};
	DebugFlags debugFlags;
};