#include "Pch.h"
#include "GameCore.h"
#include "CharacterController.h"
#include <Physics.h>
#include <BulletCollision\CollisionDispatch\btGhostObject.h>

// static helper method
static btVector3 getNormalizedVector(const btVector3& v)
{
	btVector3 n(0, 0, 0);

	if(v.length() > SIMD_EPSILON)
	{
		n = v.normalized();
	}
	return n;
}

// Returns the reflection direction of a ray going 'direction' hitting a surface with normal 'normal'
btVector3 computeReflectionDirection(const btVector3& direction, const btVector3& normal)
{
	return direction - (btScalar(2.0) * direction.dot(normal)) * normal;
}

// Returns the portion of 'direction' that is parallel to 'normal'
btVector3 parallelComponent(const btVector3& direction, const btVector3& normal)
{
	btScalar magnitude = direction.dot(normal);
	return normal * magnitude;
}

// Returns the portion of 'direction' that is perpindicular to 'normal'
btVector3 perpindicularComponent(const btVector3& direction, const btVector3& normal)
{
	return direction - parallelComponent(direction, normal);
}

class btKinematicClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
{
public:
	btKinematicClosestNotMeConvexResultCallback(btCollisionObject* me, const btVector3& up, btScalar minSlopeDot)
		: btCollisionWorld::ClosestConvexResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0)), m_me(me), m_up(up), m_minSlopeDot(minSlopeDot)
	{
	}

	virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace)
	{
		if(convexResult.m_hitCollisionObject == m_me)
			return btScalar(1.0);

		if(!convexResult.m_hitCollisionObject->hasContactResponse())
			return btScalar(1.0);

		btVector3 hitNormalWorld;
		if(normalInWorldSpace)
		{
			hitNormalWorld = convexResult.m_hitNormalLocal;
		}
		else
		{
			///need to transform normal into worldspace
			hitNormalWorld = convexResult.m_hitCollisionObject->getWorldTransform().getBasis() * convexResult.m_hitNormalLocal;
		}

		btScalar dotUp = m_up.dot(hitNormalWorld);
		if(dotUp < m_minSlopeDot)
		{
			return btScalar(1.0);
		}

		return ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
	}

protected:
	btCollisionObject* m_me;
	const btVector3 m_up;
	btScalar m_minSlopeDot;
};

CharacterController::CharacterController(float radius, float height)
{
	world = app::physics->GetWorld();

	ghost_callback = new btGhostPairCallback;
	world->getPairCache()->setInternalGhostPairCallback(ghost_callback);

	btCapsuleShape* shape = new btCapsuleShape(radius, height - radius * 2);
	app::physics->AddShape(shape);

	m_ghostObject = new btPairCachingGhostObject;
	m_ghostObject->setCollisionShape(shape);
	m_ghostObject->getWorldTransform().setOrigin(btVector3(0, height / 2, 0));
	m_ghostObject->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
	world->addCollisionObject(m_ghostObject);

	m_up.setValue(0.0f, 1.0f, 0.0f);
	m_jumpAxis.setValue(0.0f, 1.0f, 0.0f);
	m_addedMargin = 0.02f;
	m_walkDirection.setValue(0.0, 0.0, 0.0);
	m_useGhostObjectSweepTest = true;
	m_convexShape = shape;
	m_velocityTimeInterval = 0.0;
	m_verticalVelocity = 0.0;
	m_verticalOffset = 0.0;
	m_gravity = G * 3;  // 3G acceleration.
	m_fallSpeed = 55.0;     // Terminal velocity of a sky diver in m/s.
	m_jumpSpeed = 11.0f;     // ?
	m_SetjumpSpeed = m_jumpSpeed;
	m_wasOnGround = false;
	m_wasJumping = false;
	m_interpolateUp = true;
	m_currentStepOffset = 0.0;
	m_maxPenetrationDepth = 0.2f;
	m_linearDamping = btScalar(0.0);
	m_horizontalVelocity.setValue(0, 0, 0);
	prevent_fall = false;

	setUp(btVector3(0, 1, 0));
	setStepHeight(0.3f);
	setMaxSlope(btRadians(45.0));
}

CharacterController::~CharacterController()
{
	world->removeCollisionObject(m_ghostObject);
	world->getPairCache()->setInternalGhostPairCallback(nullptr);
	delete ghost_callback;
	delete m_ghostObject;
}

bool CharacterController::recoverFromPenetration()
{
	// Here we must refresh the overlapping paircache as the penetrating movement itself or the
	// previous recovery iteration might have used setWorldTransform and pushed us into an object
	// that is not in the previous cache contents from the last timestep, as will happen if we
	// are pushed into a new AABB overlap. Unhandled this means the next convex sweep gets stuck.
	//
	// Do this by calling the broadphase's setAabb with the moved AABB, this will update the broadphase
	// paircache and the ghostobject's internal paircache at the same time.    /BW

	btVector3 minAabb, maxAabb;
	m_convexShape->getAabb(m_ghostObject->getWorldTransform(), minAabb, maxAabb);
	world->getBroadphase()->setAabb(m_ghostObject->getBroadphaseHandle(),
		minAabb,
		maxAabb,
		world->getDispatcher());

	bool penetration = false;

	world->getDispatcher()->dispatchAllCollisionPairs(m_ghostObject->getOverlappingPairCache(), world->getDispatchInfo(), world->getDispatcher());

	m_currentPosition = m_ghostObject->getWorldTransform().getOrigin();

	//	btScalar maxPen = btScalar(0.0);
	for(int i = 0; i < m_ghostObject->getOverlappingPairCache()->getNumOverlappingPairs(); i++)
	{
		m_manifoldArray.resize(0);

		btBroadphasePair* collisionPair = &m_ghostObject->getOverlappingPairCache()->getOverlappingPairArray()[i];

		btCollisionObject* obj0 = static_cast<btCollisionObject*>(collisionPair->m_pProxy0->m_clientObject);
		btCollisionObject* obj1 = static_cast<btCollisionObject*>(collisionPair->m_pProxy1->m_clientObject);

		if((obj0 && !obj0->hasContactResponse()) || (obj1 && !obj1->hasContactResponse()))
			continue;

		if(!needsCollision(obj0, obj1))
			continue;

		if(collisionPair->m_algorithm)
			collisionPair->m_algorithm->getAllContactManifolds(m_manifoldArray);

		for(int j = 0; j < m_manifoldArray.size(); j++)
		{
			btPersistentManifold* manifold = m_manifoldArray[j];
			btScalar directionSign = manifold->getBody0() == m_ghostObject ? btScalar(-1.0) : btScalar(1.0);
			for(int p = 0; p < manifold->getNumContacts(); p++)
			{
				const btManifoldPoint& pt = manifold->getContactPoint(p);

				btScalar dist = pt.getDistance();

				if(dist < -m_maxPenetrationDepth)
				{
					m_currentPosition += pt.m_normalWorldOnB * directionSign * dist * btScalar(0.2);
					penetration = true;
				}
			}
		}
	}
	btTransform newTrans = m_ghostObject->getWorldTransform();
	newTrans.setOrigin(m_currentPosition);
	m_ghostObject->setWorldTransform(newTrans);
	return penetration;
}

void CharacterController::stepUp()
{
	btScalar stepHeight = 0.0f;
	if(m_verticalVelocity < 0.0)
		stepHeight = m_stepHeight;

	// phase 1: up
	btTransform start, end;

	start.setIdentity();
	end.setIdentity();

	/* FIXME: Handle penetration properly */
	start.setOrigin(m_currentPosition);

	m_targetPosition = m_currentPosition + m_up * (stepHeight)+m_jumpAxis * ((m_verticalOffset > 0.f ? m_verticalOffset : 0.f));
	m_currentPosition = m_targetPosition;

	end.setOrigin(m_targetPosition);

	btKinematicClosestNotMeConvexResultCallback callback(m_ghostObject, -m_up, m_maxSlopeCosine);
	callback.m_collisionFilterGroup = m_ghostObject->getBroadphaseHandle()->m_collisionFilterGroup;
	callback.m_collisionFilterMask = m_ghostObject->getBroadphaseHandle()->m_collisionFilterMask;

	if(m_useGhostObjectSweepTest)
	{
		m_ghostObject->convexSweepTest(m_convexShape, start, end, callback, world->getDispatchInfo().m_allowedCcdPenetration);
	}
	else
	{
		world->convexSweepTest(m_convexShape, start, end, callback, world->getDispatchInfo().m_allowedCcdPenetration);
	}

	if(callback.hasHit() && m_ghostObject->hasContactResponse() && needsCollision(m_ghostObject, callback.m_hitCollisionObject))
	{
		// Only modify the position if the hit was a slope and not a wall or ceiling.
		if(callback.m_hitNormalWorld.dot(m_up) > 0.0)
		{
			// we moved up only a fraction of the step height
			m_currentStepOffset = stepHeight * callback.m_closestHitFraction;
			if(m_interpolateUp == true)
				m_currentPosition.setInterpolate3(m_currentPosition, m_targetPosition, callback.m_closestHitFraction);
			else
				m_currentPosition = m_targetPosition;
		}

		btTransform& xform = m_ghostObject->getWorldTransform();
		xform.setOrigin(m_currentPosition);
		m_ghostObject->setWorldTransform(xform);

		// fix penetration if we hit a ceiling for example
		int numPenetrationLoops = 0;
		m_touchingContact = false;
		while(recoverFromPenetration())
		{
			numPenetrationLoops++;
			m_touchingContact = true;
			if(numPenetrationLoops > 4)
				break;
		}
		m_targetPosition = m_ghostObject->getWorldTransform().getOrigin();
		m_currentPosition = m_targetPosition;

		if(m_verticalOffset > 0)
		{
			m_verticalOffset = 0.0;
			m_verticalVelocity = 0.0;
			m_currentStepOffset = m_stepHeight;
		}
	}
	else
	{
		m_currentStepOffset = stepHeight;
		m_currentPosition = m_targetPosition;
	}
}

bool CharacterController::needsCollision(const btCollisionObject* body0, const btCollisionObject* body1)
{
	bool collides = (body0->getBroadphaseHandle()->m_collisionFilterGroup & body1->getBroadphaseHandle()->m_collisionFilterMask) != 0;
	collides = collides && (body1->getBroadphaseHandle()->m_collisionFilterGroup & body0->getBroadphaseHandle()->m_collisionFilterMask);
	return collides;
}

void CharacterController::updateTargetPositionBasedOnCollision(const btVector3& hitNormal, btScalar tangentMag, btScalar normalMag)
{
	btVector3 movementDirection = m_targetPosition - m_currentPosition;
	btScalar movementLength = movementDirection.length();
	if(movementLength > SIMD_EPSILON)
	{
		movementDirection.normalize();

		btVector3 reflectDir = computeReflectionDirection(movementDirection, hitNormal);
		reflectDir.normalize();

		btVector3 parallelDir, perpindicularDir;

		parallelDir = parallelComponent(reflectDir, hitNormal);
		perpindicularDir = perpindicularComponent(reflectDir, hitNormal);

		m_targetPosition = m_currentPosition;

		if(normalMag != 0.0)
		{
			btVector3 perpComponent = perpindicularDir * btScalar(normalMag * movementLength);
			m_targetPosition += perpComponent;
		}
	}
}

void CharacterController::stepForwardAndStrafe(const btVector3& walkMove)
{
	// phase 2: forward and strafe
	btTransform start, end;

	m_targetPosition = m_currentPosition + walkMove;

	start.setIdentity();
	end.setIdentity();

	btScalar fraction = 1.0;
	btScalar distance2 = (m_currentPosition - m_targetPosition).length2();

	int maxIter = 10;

	while(fraction > btScalar(0.01) && maxIter-- > 0)
	{
		start.setOrigin(m_currentPosition);
		end.setOrigin(m_targetPosition);
		btVector3 sweepDirNegative(m_currentPosition - m_targetPosition);

		btKinematicClosestNotMeConvexResultCallback callback(m_ghostObject, sweepDirNegative, btScalar(0.0));
		callback.m_collisionFilterGroup = m_ghostObject->getBroadphaseHandle()->m_collisionFilterGroup;
		callback.m_collisionFilterMask = m_ghostObject->getBroadphaseHandle()->m_collisionFilterMask;

		btScalar margin = m_convexShape->getMargin();
		m_convexShape->setMargin(margin + m_addedMargin);

		if(!(start == end))
		{
			if(m_useGhostObjectSweepTest)
			{
				m_ghostObject->convexSweepTest(m_convexShape, start, end, callback, world->getDispatchInfo().m_allowedCcdPenetration);
			}
			else
			{
				world->convexSweepTest(m_convexShape, start, end, callback, world->getDispatchInfo().m_allowedCcdPenetration);
			}
		}
		m_convexShape->setMargin(margin);

		fraction -= callback.m_closestHitFraction;

		if(callback.hasHit() && m_ghostObject->hasContactResponse() && needsCollision(m_ghostObject, callback.m_hitCollisionObject))
		{
			// we moved only a fraction
			updateTargetPositionBasedOnCollision(callback.m_hitNormalWorld);
			btVector3 currentDir = m_targetPosition - m_currentPosition;
			distance2 = currentDir.length2();
			if(distance2 > SIMD_EPSILON)
			{
				currentDir.normalize();
				/* See Quake2: "If velocity is against original velocity, stop ead to avoid tiny oscilations in sloping corners." */
				if(currentDir.dot(m_normalizedDirection) <= btScalar(0.0))
					break;
			}
			else
				break;
		}
		else
		{
			m_currentPosition = m_targetPosition;
		}
	}
}

void CharacterController::stepDown(btScalar dt)
{
	btTransform start, end, end_double;
	bool runonce = false;

	// phase 3: down

	btVector3 orig_position = m_targetPosition;

	btScalar downVelocity = (m_verticalVelocity < 0.f ? -m_verticalVelocity : 0.f) * dt;

	if(m_verticalVelocity > 0.0)
		return;

	if(downVelocity > 0.0 && downVelocity > m_fallSpeed && (m_wasOnGround || !m_wasJumping))
		downVelocity = m_fallSpeed;

	btVector3 step_drop = m_up * (m_currentStepOffset + downVelocity);
	m_targetPosition -= step_drop;

	btKinematicClosestNotMeConvexResultCallback callback(m_ghostObject, m_up, m_maxSlopeCosine);
	callback.m_collisionFilterGroup = m_ghostObject->getBroadphaseHandle()->m_collisionFilterGroup;
	callback.m_collisionFilterMask = m_ghostObject->getBroadphaseHandle()->m_collisionFilterMask;

	btKinematicClosestNotMeConvexResultCallback callback2(m_ghostObject, m_up, m_maxSlopeCosine);
	callback2.m_collisionFilterGroup = m_ghostObject->getBroadphaseHandle()->m_collisionFilterGroup;
	callback2.m_collisionFilterMask = m_ghostObject->getBroadphaseHandle()->m_collisionFilterMask;

	while(1)
	{
		start.setIdentity();
		end.setIdentity();

		end_double.setIdentity();

		start.setOrigin(m_currentPosition);
		end.setOrigin(m_targetPosition);

		//set double test for 2x the step drop, to check for a large drop vs small drop
		end_double.setOrigin(m_targetPosition - step_drop);

		if(m_useGhostObjectSweepTest)
		{
			m_ghostObject->convexSweepTest(m_convexShape, start, end, callback, world->getDispatchInfo().m_allowedCcdPenetration);

			if(!callback.hasHit() && m_ghostObject->hasContactResponse())
			{
				//test a double fall height, to see if the character should interpolate it's fall (full) or not (partial)
				m_ghostObject->convexSweepTest(m_convexShape, start, end_double, callback2, world->getDispatchInfo().m_allowedCcdPenetration);
			}
		}
		else
		{
			world->convexSweepTest(m_convexShape, start, end, callback, world->getDispatchInfo().m_allowedCcdPenetration);

			if(!callback.hasHit() && m_ghostObject->hasContactResponse())
			{
				//test a double fall height, to see if the character should interpolate it's fall (large) or not (small)
				world->convexSweepTest(m_convexShape, start, end_double, callback2, world->getDispatchInfo().m_allowedCcdPenetration);
			}
		}

		btScalar downVelocity2 = (m_verticalVelocity < 0.f ? -m_verticalVelocity : 0.f) * dt;
		bool has_hit = callback2.hasHit() && m_ghostObject->hasContactResponse() && needsCollision(m_ghostObject, callback2.m_hitCollisionObject);

		btScalar stepHeight = 0.0f;
		if(m_verticalVelocity < 0.0)
			stepHeight = m_stepHeight;

		if(downVelocity2 > 0.0 && downVelocity2 < stepHeight && has_hit == true && runonce == false && (m_wasOnGround || !m_wasJumping))
		{
			//redo the velocity calculation when falling a small amount, for fast stairs motion
			//for larger falls, use the smoother/slower interpolated movement by not touching the target position

			m_targetPosition = orig_position;
			downVelocity = stepHeight;

			step_drop = m_up * (m_currentStepOffset + downVelocity);
			m_targetPosition -= step_drop;
			runonce = true;
			continue;  //re-run previous tests
		}
		break;
	}

	if((m_ghostObject->hasContactResponse() && (callback.hasHit() && needsCollision(m_ghostObject, callback.m_hitCollisionObject))) || runonce == true)
	{
		// we dropped a fraction of the height -> hit floor
		//btScalar fraction = (m_currentPosition.getY() - callback.m_hitPointWorld.getY()) / 2;

		m_currentPosition.setInterpolate3(m_currentPosition, m_targetPosition, callback.m_closestHitFraction);

		m_verticalVelocity = 0.0;
		m_verticalOffset = 0.0;
		m_wasJumping = false;
	}
	else
	{
		// we dropped the full height
		if(prevent_fall)
		{
			// TODO
		}

		m_currentPosition = m_targetPosition;
	}
}

void CharacterController::setWalkDirection(const btVector3& walkDirection)
{
	m_walkDirection = walkDirection;
	m_normalizedDirection = getNormalizedVector(m_walkDirection);
}

void CharacterController::reset()
{
	m_verticalVelocity = 0.0;
	m_verticalOffset = 0.0;
	m_wasOnGround = false;
	m_wasJumping = false;
	m_walkDirection.setValue(0, 0, 0);
	m_velocityTimeInterval = 0.0;
	m_horizontalVelocity.setValue(0, 0, 0);

	//clear pair cache
	btHashedOverlappingPairCache* cache = m_ghostObject->getOverlappingPairCache();
	while(cache->getOverlappingPairArray().size() > 0)
	{
		cache->removeOverlappingPair(cache->getOverlappingPairArray()[0].m_pProxy0, cache->getOverlappingPairArray()[0].m_pProxy1, world->getDispatcher());
	}
}

void CharacterController::warp(const btVector3& origin)
{
	btTransform xform;
	xform.setIdentity();
	xform.setOrigin(origin);
	m_ghostObject->setWorldTransform(xform);
}

void CharacterController::update(btScalar dt)
{
	m_currentPosition = m_ghostObject->getWorldTransform().getOrigin();
	m_targetPosition = m_currentPosition;
	m_wasOnGround = onGround();

	// damping & apply horizontal velocity
	m_horizontalVelocity = m_horizontalVelocity * (1.f - dt * m_linearDamping) + m_walkDirection * (dt * m_linearDamping);

	// Update fall velocity.
	m_verticalVelocity -= m_gravity * dt;
	if(m_verticalVelocity > 0.0 && m_verticalVelocity > m_jumpSpeed)
	{
		m_verticalVelocity = m_jumpSpeed;
	}
	if(m_verticalVelocity < 0.0 && btFabs(m_verticalVelocity) > btFabs(m_fallSpeed))
	{
		m_verticalVelocity = -btFabs(m_fallSpeed);
	}
	m_verticalOffset = m_verticalVelocity * dt;

	btTransform xform;
	xform = m_ghostObject->getWorldTransform();

	stepUp();

	stepForwardAndStrafe(m_horizontalVelocity * dt);

	stepDown(dt);

	xform.setOrigin(m_currentPosition);
	m_ghostObject->setWorldTransform(xform);

	int numPenetrationLoops = 0;
	m_touchingContact = false;
	while(recoverFromPenetration())
	{
		numPenetrationLoops++;
		m_touchingContact = true;
		if(numPenetrationLoops > 4)
			break;
	}
}

void CharacterController::setFallSpeed(btScalar fallSpeed)
{
	m_fallSpeed = fallSpeed;
}

void CharacterController::setJumpSpeed(btScalar jumpSpeed)
{
	m_jumpSpeed = jumpSpeed;
	m_SetjumpSpeed = m_jumpSpeed;
}

bool CharacterController::canJump() const
{
	return onGround();
}

void CharacterController::jump()
{
	m_jumpSpeed = m_SetjumpSpeed;
	m_verticalVelocity = m_jumpSpeed;
	m_wasJumping = true;

	m_jumpAxis = m_up;

	m_jumpPosition = m_ghostObject->getWorldTransform().getOrigin();
}

void CharacterController::setGravity(const btVector3& gravity)
{
	if(gravity.length2() > 0) setUpVector(-gravity);

	m_gravity = gravity.length();
}

btVector3 CharacterController::getGravity() const
{
	return -m_gravity * m_up;
}

void CharacterController::setMaxSlope(btScalar slopeRadians)
{
	m_maxSlopeRadians = slopeRadians;
	m_maxSlopeCosine = btCos(slopeRadians);
}

btScalar CharacterController::getMaxSlope() const
{
	return m_maxSlopeRadians;
}

bool CharacterController::onGround() const
{
	return (fabs(m_verticalVelocity) < SIMD_EPSILON) && (fabs(m_verticalOffset) < SIMD_EPSILON);
}

void CharacterController::setStepHeight(btScalar h)
{
	m_stepHeight = h;
}

void CharacterController::setUpInterpolate(bool value)
{
	m_interpolateUp = value;
}

void CharacterController::setUp(const btVector3& up)
{
	if(up.length2() > 0 && m_gravity > 0.0f)
	{
		setGravity(-m_gravity * up.normalized());
		return;
	}

	setUpVector(up);
}

void CharacterController::setUpVector(const btVector3& up)
{
	if(m_up == up)
		return;

	btVector3 u = m_up;

	if(up.length2() > 0)
		m_up = up.normalized();
	else
		m_up = btVector3(0.0, 0.0, 0.0);

	if(!m_ghostObject) return;
	btQuaternion rot = getRotation(m_up, u);

	//set orientation with new up
	btTransform xform;
	xform = m_ghostObject->getWorldTransform();
	btQuaternion orn = rot.inverse() * xform.getRotation();
	xform.setRotation(orn);
	m_ghostObject->setWorldTransform(xform);
}

btQuaternion CharacterController::getRotation(btVector3& v0, btVector3& v1) const
{
	if(v0.length2() == 0.0f || v1.length2() == 0.0f)
	{
		btQuaternion q;
		return q;
	}

	return shortestArcQuatNormalize2(v0, v1);
}
