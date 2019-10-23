#pragma once

#include <btBulletCollisionCommon.h>

class btPairCachingGhostObject;
class btGhostPairCallback;

// based on btKinematicCharacterController.h
ATTRIBUTE_ALIGNED16(struct) CharacterController
{
protected:
	btCollisionWorld* world;
	btPairCachingGhostObject* m_ghostObject;
	btGhostPairCallback* ghost_callback;
	btConvexShape* m_convexShape;  //is also in m_ghostObject, but it needs to be convex, so we store it here to avoid upcast

	btScalar m_maxPenetrationDepth;
	btScalar m_verticalVelocity;
	btScalar m_verticalOffset;
	btScalar m_fallSpeed;
	btScalar m_jumpSpeed;
	btScalar m_SetjumpSpeed;
	btScalar m_maxSlopeRadians;  // Slope angle that is set (used for returning the exact value)
	btScalar m_maxSlopeCosine;   // Cosine equivalent of m_maxSlopeRadians (calculated once when set, for optimization)
	btScalar m_gravity;

	btScalar m_stepHeight;

	btScalar m_addedMargin;  //@todo: remove this and fix the code

	///this is the desired walk direction, set by the user
	btVector3 m_walkDirection;
	btVector3 m_normalizedDirection;
	btVector3 m_horizontalVelocity;

	btVector3 m_jumpPosition;

	//some internal variables
	btVector3 m_currentPosition;
	btScalar m_currentStepOffset;
	btVector3 m_targetPosition;

	///keep track of the contact manifolds
	btManifoldArray m_manifoldArray;

	bool m_touchingContact;
	btVector3 m_touchingNormal;

	btScalar m_linearDamping;

	bool m_wasOnGround;
	bool m_wasJumping;
	bool m_useGhostObjectSweepTest;
	btScalar m_velocityTimeInterval;
	btVector3 m_up;
	btVector3 m_jumpAxis;

	bool m_interpolateUp;
	bool prevent_fall;

	bool recoverFromPenetration();
	void stepUp();
	void updateTargetPositionBasedOnCollision(const btVector3 & hit_normal, btScalar tangentMag = btScalar(0.0), btScalar normalMag = btScalar(1.0));
	void stepForwardAndStrafe(const btVector3 & walkMove);
	void stepDown(btScalar dt);

	bool needsCollision(const btCollisionObject * body0, const btCollisionObject * body1);

	void setUpVector(const btVector3 & up);

	btQuaternion getRotation(btVector3 & v0, btVector3 & v1) const;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	CharacterController(float radius, float height);
	~CharacterController();

	void update(btScalar deltaTime);

	void setUp(const btVector3 & up);

	const btVector3& getUp() { return m_up; }

	/// This should probably be called setPositionIncrementPerSimulatorStep.
	/// This is neither a direction nor a velocity, but the amount to
	///	increment the position each simulation iteration, regardless
	///	of dt.
	/// This call will reset any velocity set by setVelocityForTimeInterval().
	void setWalkDirection(const btVector3 & walkDirection);

	void setLinearDamping(btScalar d) { m_linearDamping = d; }
	btScalar getLinearDamping() const { return m_linearDamping; }
	const btVector3& getVelocity() const { return m_horizontalVelocity; }

	void reset();
	void warp(const btVector3 & origin);

	void setStepHeight(btScalar h);
	btScalar getStepHeight() const { return m_stepHeight; }
	void setFallSpeed(btScalar fallSpeed);
	btScalar getFallSpeed() const { return m_fallSpeed; }
	void setJumpSpeed(btScalar jumpSpeed);
	btScalar getJumpSpeed() const { return m_jumpSpeed; }
	bool canJump() const;

	void jump();

	void setGravity(const btVector3 & gravity);
	btVector3 getGravity() const;

	/// The max slope determines the maximum angle that the controller can walk up.
	/// The slope angle is measured in radians.
	void setMaxSlope(btScalar slopeRadians);
	btScalar getMaxSlope() const;

	btCollisionObject* getObject() { return (btCollisionObject*)m_ghostObject; }

	bool onGround() const;
	void setUpInterpolate(bool value);

	void setPreventFall(bool value) { prevent_fall = value; }
	const btVector3& getPos() const { return m_currentPosition; }
};
