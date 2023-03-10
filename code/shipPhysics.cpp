#include "shipPhysics.h"

#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"

ShipPhysics::ShipPhysics(btConvexShape *convexShape, btPairCachingGhostObject *ghostObject, ShipConfig &shipConfig)
	: m_convexShape(convexShape), m_ghostObject(ghostObject), m_shipConfig(shipConfig)
{
	m_thrustInput = btVector3(0.0f, 0.0f, 0.0f);
	m_rotationInput = btVector3(0.0f, 0.0f, 0.0f);
	m_position = btVector3(0.0f, 0.0f, 0.0f);
	m_velocity = btVector3(0.0f, 0.0f, 0.0f);
	m_rotation = btQuaternion::getIdentity();
}

static bool CollisionMasksCollides(const btCollisionObject *obj1, const btCollisionObject *obj2)
{
	bool maskCollides = (obj1->getBroadphaseHandle()->m_collisionFilterGroup & obj2->getBroadphaseHandle()->m_collisionFilterMask) != 0;
	maskCollides = maskCollides && ((obj2->getBroadphaseHandle()->m_collisionFilterGroup & obj1->getBroadphaseHandle()->m_collisionFilterMask) != 0);
	return maskCollides;
}

static btVector3 ComputeReflectionDirection(const btVector3 &direction, const btVector3 &normal)
{
	return direction - (btScalar(2.0) * direction.dot(normal)) * normal;
}

//static btVector3 ParallelComponent(const btVector3 &direction, const btVector3 &normal)
//{
//	btScalar magnitude = direction.dot(normal);
//	return normal * magnitude;
//}
//
//static btVector3 PerpindicularComponent(const btVector3 &direction, const btVector3 &normal)
//{
//	return direction - ParallelComponent(direction, normal);
//}

void ShipPhysics::updateAction(btCollisionWorld *collisionWorld, btScalar deltaTimeStep)
{
	OPTICK_EVENT();

	r32 pitchRadians = m_rotationInput.getX() * m_shipConfig.pitchRate * DEG2RAD * deltaTimeStep;
	r32 yawRadians = m_rotationInput.getY() * m_shipConfig.yawRate * DEG2RAD * deltaTimeStep;
	r32 rollRadians = m_rotationInput.getZ() * m_shipConfig.rollRate * DEG2RAD * deltaTimeStep;

	btQuaternion rotationDelta(yawRadians, pitchRadians, rollRadians);
	btQuaternion targetRotation = m_rotation * rotationDelta;

	btVector3 rotatedVelocity = quatRotate(m_rotation, m_velocity);
	btVector3 targetPosition = m_position + (rotatedVelocity * deltaTimeStep);

	// Friction forces to any velocity other than foward of back
	m_velocity *= btVector3(0.95f, 0.95f, 0.99f); // TODO: need to be improved how I handle this

	// Add acceleration/thrust
	btVector3 thrustVector = m_thrustInput * m_shipConfig.thrustSpeed * deltaTimeStep;
	btVector3 targetVelocity = m_velocity + thrustVector;


	btCollisionWorld::ClosestConvexResultCallback sweepResultCallback(btVector3(0.0f, 0.0f, 0.0f), btVector3(0.0f, 0.0f, 0.0f));
	sweepResultCallback.m_collisionFilterGroup = m_ghostObject->getBroadphaseHandle()->m_collisionFilterGroup;
	sweepResultCallback.m_collisionFilterMask = m_ghostObject->getBroadphaseHandle()->m_collisionFilterMask;


	btTransform start = btTransform::getIdentity();
	start.setOrigin(m_position);
	start.setRotation(m_rotation);

	btTransform end = btTransform::getIdentity();
	end.setOrigin(targetPosition);
	end.setRotation(targetRotation);

	collisionWorld->convexSweepTest(m_convexShape, start, end, sweepResultCallback, collisionWorld->getDispatchInfo().m_allowedCcdPenetration);

	if(sweepResultCallback.hasHit() && m_ghostObject->hasContactResponse() && CollisionMasksCollides(m_ghostObject, sweepResultCallback.m_hitCollisionObject))
	{
		btVector3 movementVector = targetPosition - m_position;
		btScalar movementLength = movementVector.length();
		if(movementLength > SIMD_EPSILON)
		{
			btVector3 movementDirection = movementVector.normalized();

			btVector3 reflectDir = ComputeReflectionDirection(movementDirection, sweepResultCallback.m_hitNormalWorld);
			reflectDir.normalize();

			btScalar velocityTotal = targetVelocity.length();

			btScalar collisionSlowdownFract = btFabs(movementDirection.dot(sweepResultCallback.m_hitNormalWorld));
			btScalar slowDownAmount = velocityTotal * collisionSlowdownFract;
			targetVelocity = reflectDir * (velocityTotal - slowDownAmount);

			btScalar impulseOnOtherObject = (collisionSlowdownFract);
			if(sweepResultCallback.m_hitCollisionObject->getInternalType() == btCollisionObject::CO_RIGID_BODY)
			{
				btRigidBody *otherBody = btRigidBody::upcast((btCollisionObject *)sweepResultCallback.m_hitCollisionObject);

				btScalar normalImpulse = 10.0f;
				btVector3 impulseVec = -sweepResultCallback.m_hitNormalWorld * (normalImpulse);
				btVector3 rel_pos1 = sweepResultCallback.m_hitPointWorld - otherBody->getWorldTransform().getOrigin();
				otherBody->activate();
				otherBody->applyImpulse(impulseVec, rel_pos1);
			}
		}
	}

	m_position = targetPosition;
	m_velocity = targetVelocity;
	m_rotation = targetRotation;

	m_thrustInput = btVector3(0.0, 0.0, 0.0f);
	m_rotationInput = btVector3(0.0, 0.0, 0.0f);

	btTransform resultTransform = btTransform::getIdentity();
	resultTransform.setOrigin(m_position);
	resultTransform.setRotation(m_rotation);
	m_ghostObject->setWorldTransform(resultTransform);
}

void ShipPhysics::debugDraw(btIDebugDraw *debugDrawer)
{
	btVector3 rotatedVelocity = quatRotate(m_rotation, m_velocity);
	btVector3 relVelocity = m_position + rotatedVelocity;

	debugDrawer->drawLine(m_position, relVelocity, btVector3(0.0f, 1.0f, 0.0f));
}

void ShipPhysics::ApplyShipInput(const ShipInput &shipInput)
{
	r32 thrustX = shipInput.inputs[ShipInput::THRUST_X];
	r32 thrustY = shipInput.inputs[ShipInput::THRUST_Y];
	r32 thrustZ = shipInput.inputs[ShipInput::THRUST_Z];

	if(thrustX != 0.0f || thrustY != 0.0f || thrustZ != 0.0f)
	{
		m_thrustInput = btVector3(thrustX, thrustY, thrustZ).normalize();
	}

	// Rotation
	r32 pitch = shipInput.inputs[ShipInput::PITCH];
	r32 yaw = shipInput.inputs[ShipInput::YAW];
	r32 roll = shipInput.inputs[ShipInput::ROLL];
	if(pitch != 0.0f || yaw != 0.0f || roll != 0.0f)
	{
		m_rotationInput = btVector3(pitch, yaw, roll);
	}
}

Transform ShipPhysics::GetTransform() const
{
	Transform transform;
	transform.translation = m_position;
	transform.rotation = m_rotation;
	return transform;
}

btVector3 ShipPhysics::GetVelocity() const
{
	return m_velocity;
}