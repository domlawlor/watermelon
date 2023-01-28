#pragma once

#include "defines.h"

#include "game.h"

#include "BulletDynamics/Dynamics/btActionInterface.h"

#include "raylib.h"

class btConvexShape;
class btPairCachingGhostObject;

class ShipPhysics : public btActionInterface
{
public:
	ShipPhysics() = delete;
	ShipPhysics(btConvexShape *convexShape, btPairCachingGhostObject *ghostObject, ShipConfig &shipConfig);

	~ShipPhysics() {};

	// btActionInterface methods
	void updateAction(btCollisionWorld *collisionWorld, btScalar deltaTimeStep) override;
	void debugDraw(btIDebugDraw *debugDrawer) override;
	// 

	void ApplyShipInput(const ShipInput &shipInput);

	ShipConfig GetShipConfig() { return m_shipConfig; };
	void SetShipConfig(const ShipConfig &shipConfig) { m_shipConfig = shipConfig; };

	void FillTransform(Transform &transform) const;

private:
	btConvexShape *m_convexShape = nullptr;
	btPairCachingGhostObject *m_ghostObject = nullptr;

	ShipConfig m_shipConfig;

	btVector3 m_thrustInput;
	btVector3 m_rotationInput; // pitch, yaw, roll

	btVector3 m_position;
	btVector3 m_velocity;
	btQuaternion m_rotation = btQuaternion::getIdentity();
};