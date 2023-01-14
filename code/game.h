#pragma once

#include "defines.h"

#include "external/entt.hpp"

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

	r64 m_lastFrameTime = 0;

	u32 m_windowWidth = 0;
	u32 m_windowHeight = 0;


	struct DebugFlags
	{
		bool drawCollision = false;
	};
	DebugFlags debugFlags;
};