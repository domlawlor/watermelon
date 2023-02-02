#pragma once

#include "defines.h"

#include "math.h"

#include "raylib.h"

#include <vector>

#include "LinearMath/btVector3.h"
#include "LinearMath/btQuaternion.h"

class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;

class btPairCachingGhostObject;

class btConvexShape;
class btConvexHullShape;
class btRigidBody;

class CollisionDrawer;

class btActionInterface;

struct RigidBody
{
	btRigidBody *body;
};

class PhysicsWorld
{
public:
	PhysicsWorld() { InitWorld(); };
	~PhysicsWorld() { FreeWorld(); };

	void Step(r32 deltaTime);

	btPairCachingGhostObject *CreateGhostObject(btConvexShape *collisionShape, int filterGroup, int filterMask);

	RigidBody CreateRigidBody(btVector3 position, btQuaternion rotation, r32 mass, btConvexShape *collisionShape);

	void AddAction(btActionInterface *action);

	void DrawDebugInfo();


private:
	void InitWorld();
	void FreeWorld();

	btDefaultCollisionConfiguration *m_collisionConfiguration;
	btCollisionDispatcher *m_dispatcher;
	btBroadphaseInterface *m_broadphase;
	btSequentialImpulseConstraintSolver *m_solver;
	btDiscreteDynamicsWorld *m_world;

	CollisionDrawer *m_debugDrawer;
};



btConvexShape * CreateCapsuleXAxisCollision(r32 radius, r32 length);
btConvexShape * CreateCapsuleYAxisCollision(r32 radius, r32 length);
btConvexShape * CreateCapsuleZAxisCollision(r32 radius, r32 length);

btConvexShape *CreateCylinderXAxisCollision(r32 radius, r32 length);
btConvexShape *CreateCylinderYAxisCollision(r32 radius, r32 length);
btConvexShape *CreateCylinderZAxisCollision(r32 radius, r32 length);


btConvexShape * CreateConvexCollision(const Raylib::Model &model, r32 scale);