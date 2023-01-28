#include "physics.h"

#include "game.h"

#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"


#include "raylib.h"
#include "raymath.h"

#include <vector>

class CollisionDrawer : public btIDebugDraw
{
public:
	void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override
	{
		const Vector3 source = {from.getX(), from.getY(), from.getZ()};
		const Vector3 dest = {to.getX(), to.getY(), to.getZ()};
		Color colorInBytes;
		colorInBytes.r = (u8)(color.getX() * 255.0f);
		colorInBytes.g = (u8)(color.getY() * 255.0f); 
		colorInBytes.b = (u8)(color.getZ() * 255.0f);
		DrawLine3D(source, dest, colorInBytes);
	}

	void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) override
	{

	}

	void reportErrorWarning(const char *warningString) override { TraceLog(LOG_ERROR, warningString); }

	void draw3dText(const btVector3 &location, const char *textString) override
	{
		
	}

	void setDebugMode(int debugMode) override { m_debugMode = debugMode; }
	int getDebugMode() const override { return m_debugMode; }

private:
	int m_debugMode = DBG_NoDebug;
};

void PhysicsWorld::InitWorld()
{
	OPTICK_EVENT();
	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
	m_broadphase = new btDbvtBroadphase();
	m_solver = new btSequentialImpulseConstraintSolver();
	m_world = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfiguration);
	
	CollisionDrawer *m_debugDrawer = new CollisionDrawer();
	//m_debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawAabb);
	//m_debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawContactPoints);
	//m_debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
	m_debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawAabb | btIDebugDraw::DBG_DrawWireframe);
	m_world->setDebugDrawer(m_debugDrawer);

	m_world->setGravity(btVector3(0.0, 0.0, 0.0));
}

void PhysicsWorld::FreeWorld()
{
	OPTICK_EVENT();

	delete m_debugDrawer;

	delete m_world;
	delete m_solver;
	delete m_dispatcher;
	delete m_collisionConfiguration;
	delete m_broadphase;
}

void PhysicsWorld::Step(r32 deltaTime)
{
	OPTICK_EVENT();
	m_world->stepSimulation(deltaTime);
}

inline void TransformToBtTransform(const Transform &tranform, btTransform &outBodyTransform)
{
	btVector3 origin(tranform.translation.x, tranform.translation.y, tranform.translation.z);
	btQuaternion rotation(tranform.rotation.x, tranform.rotation.y, tranform.rotation.z, tranform.rotation.w);
	outBodyTransform = btTransform(rotation, origin);
}

inline void BtTransformToTransform(const btTransform &bodyTranform, Transform &outTransform)
{
	btVector3 origin = bodyTranform.getOrigin();

	outTransform.translation.x = origin.getX();
	outTransform.translation.y = origin.getY();
	outTransform.translation.z = origin.getZ();

	//btQuaternion rotation = bodyTranform.getRotation();
	//outTransform.rotation.x = rotation.getX();
	//outTransform.rotation.y = rotation.getY();
	//outTransform.rotation.z = rotation.getZ();
	//outTransform.rotation.w = rotation.getW();
}

btPairCachingGhostObject *PhysicsWorld::CreateGhostObject(btConvexShape *collisionShape)
{
	btPairCachingGhostObject *ghostObject = new btPairCachingGhostObject();
	ghostObject->setCollisionShape(collisionShape);
	m_world->addCollisionObject(ghostObject);
	return ghostObject;
}

RigidBody PhysicsWorld::CreateRigidBody(Vector3 position, Quaternion rotation, r32 mass, btConvexShape *collisionShape)
{
	btVector3 bodyInertia;
	collisionShape->calculateLocalInertia(mass, bodyInertia);

	btQuaternion btRotation(rotation.x, rotation.y, rotation.z, rotation.w);
	btVector3 btPosition(position.x, position.y, position.z);
	btTransform startTransform(btRotation, btPosition);

	btTransform centerOfMassOffset = btTransform::getIdentity();
	btDefaultMotionState *motionState = new btDefaultMotionState(startTransform, centerOfMassOffset);

	btRigidBody::btRigidBodyConstructionInfo constructInfo = btRigidBody::btRigidBodyConstructionInfo(mass, motionState, collisionShape, bodyInertia);

	RigidBody rigidBody;
	rigidBody.body = new btRigidBody(constructInfo);

	m_world->addRigidBody(rigidBody.body);
	return rigidBody;
}


void PhysicsWorld::UpdateTransform(const RigidBody &rigidBody, Transform &transform)
{
	btTransform bodyTransform = rigidBody.body->getWorldTransform();

	Vector3 translationBefore = transform.translation;
	Quaternion rotationBefore = transform.rotation;

	BtTransformToTransform(bodyTransform, transform);
}


void PhysicsWorld::AddAction(btActionInterface *action)
{
	m_world->addAction(action);
}

void PhysicsWorld::DrawDebugInfo()
{
	OPTICK_EVENT();
	m_world->debugDrawWorld();
}

btConvexShape *CreateCapsuleXAxisCollision(r32 radius, r32 length)
{
	btCapsuleShape *collision = new btCapsuleShapeX(radius, length);
	return collision;
}

btConvexShape * CreateCapsuleYAxisCollision(r32 radius, r32 length)
{
	btCapsuleShape *collision = new btCapsuleShape(radius, length);
	return collision;
}

btConvexShape *CreateCapsuleZAxisCollision(r32 radius, r32 length)
{
	btCapsuleShape *collision = new btCapsuleShapeZ(radius, length);
	return collision;
}

btConvexShape *CreateCylinderXAxisCollision(r32 radius, r32 length)
{
	btVector3 halfExtents(length / 2.0f, radius, radius);
	btCylinderShape *collision = new btCylinderShapeX(halfExtents);
	return collision;
}

btConvexShape *CreateCylinderYAxisCollision(r32 radius, r32 length)
{
	btVector3 halfExtents(radius, length / 2.0f, radius);
	btCylinderShape *collision = new btCylinderShape(halfExtents);
	return collision;
}

btConvexShape *CreateCylinderZAxisCollision(r32 radius, r32 length)
{
	btVector3 halfExtents(radius, radius, length / 2.0f);
	btCylinderShape *collision = new btCylinderShapeZ(halfExtents);
	return collision;
}

btConvexShape * CreateConvexCollision(const Model &model, r32 scale)
{
	Assert(model.meshCount == 1);
	Mesh *mesh = &model.meshes[0];

	btConvexHullShape *collision = new btConvexHullShape();
	
	Assert(mesh->vertexCount > 0)

	Vector3 *vertices = (Vector3 *)mesh->vertices;
	for(int vertexNum = 0; vertexNum < mesh->vertexCount; ++vertexNum)
	{
		Vector3 *vertex = &vertices[vertexNum];
		btVector3 btv = btVector3(vertex->x, vertex->y, vertex->z);
		collision->addPoint(btv);
	}

	collision->setLocalScaling(btVector3(scale, scale, scale));

	return collision;
}

