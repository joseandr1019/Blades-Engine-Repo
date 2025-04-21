#pragma once
#include "Actor.h"
#include "SceneDB.h"

#include <algorithm>
#include <string>

#include "box2d/box2d.h"
#include "glm/glm.hpp"

const uint16_t COLLIDER_CATEGORY = 0x0001;
const uint16_t TRIGGER_CATEGORY = 0x0002;

class Rigidbody {
public:
	bool precise = true;
	bool has_collider = true;
	bool has_trigger = true;
	bool enabled = true;
	bool removed = false;
	float x = 0.0f;
	float y = 0.0f;
	float gravity_scale = 1.0f;
	float density = 1.0f;
	float angular_friction = 0.3f;
	float rotation = 0.0f;
	float width = 1.0f;
	float height = 1.0f;
	float radius = 0.5f;
	float trigger_width = 1.0f;
	float trigger_height = 1.0f;
	float trigger_radius = 0.5f;
	float friction = 0.3f;
	float bounciness = 0.3f;
	b2Body* body = nullptr;
	Actor* actor = nullptr;
	std::string body_type = "dynamic";
	std::string collider_type = "box";
	std::string trigger_type = "box";
	std::string key = "";
	std::string type = "Rigidbody";

	Rigidbody() {}
	Rigidbody(const Rigidbody* rb) {
		precise = rb->precise;
		has_collider = rb->has_collider;
		has_trigger = rb->has_trigger;
		x = rb->x;
		y = rb->y;
		gravity_scale = rb->gravity_scale;
		density = rb->density;
		angular_friction = rb->angular_friction;
		rotation = rb->rotation;
		width = rb->width;
		height = rb->height;
		radius = rb->radius;
		trigger_height = rb->trigger_height;
		trigger_radius = rb->trigger_radius;
		trigger_width = rb->trigger_width;
		friction = rb->friction;
		bounciness = rb->bounciness;
		body_type = rb->body_type;
		collider_type = rb->collider_type;
		trigger_type = rb->trigger_type;
		key = rb->key;
	}

	b2Vec2 GetPosition() {
		if (this->body) {
			return this->body->GetPosition();
		}
		else {
			return b2Vec2(this->x, this->y);
		}
	}
	float GetRotation() {
		if (this->body) {
			float radians = this->body->GetAngle();
			return radians * (180.0f / b2_pi);
		}
		else {
			return this->rotation;
		}
	}
	void AddForce(b2Vec2 f) {
		this->body->ApplyForceToCenter(f, true);
	}
	void SetVelocity(const b2Vec2& v) {
		this->body->SetLinearVelocity(v);
	}
	void SetPosition(const b2Vec2& pos) {
		if (this->body) {
			this->body->SetTransform(pos, this->body->GetAngle());
		}
		else {
			this->x = pos.x;
			this->y = pos.y;
		}
	}
	void SetRotation(float degrees) {
		if (this->body) {
			float radians = degrees * (b2_pi / 180.0f);
			this->body->SetTransform(this->body->GetPosition(), radians);
		}
		else {
			this->rotation = degrees;
		}
	}
	void SetAngularVelocity(float degrees) {
		float radians = degrees * (b2_pi / 180.0f);
		this->body->SetAngularVelocity(radians);
	}
	void SetGravityScale(float scale) {
		if (this->body) {
			this->body->SetGravityScale(scale);
		}
		else {
			this->gravity_scale = scale;
		}
	}
	void SetUpDirection(const b2Vec2& vec) {
		if (this->body) {
			b2Vec2 dir = vec;
			dir.Normalize();
			float radians = glm::atan(dir.x, -dir.y);
			this->body->SetTransform(this->body->GetPosition(), radians);
		}
		else {
			b2Vec2 dir = vec;
			float degrees = glm::atan(dir.x, -dir.y) * (180.0f / b2_pi);
			this->rotation = degrees;
		}
	}
	void SetRightDirection(const b2Vec2& vec) {
		b2Vec2 dir = vec;
		dir.Normalize();
		if (this->body) {
			float radians = glm::atan(dir.x, -dir.y) - (b2_pi / 2.0f);
			this->body->SetTransform(this->body->GetPosition(), radians);
		}
		else {
			float degrees = (glm::atan(dir.x, -dir.y) - (b2_pi / 2.0f)) * (180.0f / b2_pi);
			this->rotation = degrees;
		}
	}
	b2Vec2 GetVelocity() {
		return this->body->GetLinearVelocity();
	}
	float GetAngularVelocity() {
		return this->body->GetAngularVelocity() * (180.0f / b2_pi);
	}
	float GetGravityScale() {
		if (this->body) {
			return this->body->GetGravityScale();
		}
		else {
			return this->gravity_scale;
		}
	}
	b2Vec2 GetUpDirection() {
		float theta;
		if (this->body) {
			theta = this->body->GetAngle();
		}
		else {
			theta = this->rotation * (b2_pi / 180.0f);
		}
		b2Vec2 dir = b2Vec2(glm::sin(theta), -1.0f * glm::cos(theta));
		dir.Normalize();
		return dir;
	}
	b2Vec2 GetRightDirection() {
		float theta;
		if (this->body) {
			theta = this->body->GetAngle();
		}
		else {
			theta = this->rotation * (b2_pi / 180.0f);
		}
		b2Vec2 dir = b2Vec2(glm::cos(theta), glm::sin(theta));
		dir.Normalize();
		return dir;
	}

	void OnStart();
	void OnDestroy() {
		SceneDB::world.DestroyBody(this->body);
	}
	static void LuaInit();
};

struct Collision {
	Actor* other = nullptr;
	b2Vec2 point = b2Vec2(-999.0f, -999.0f);
	b2Vec2 relative_velocity = b2Vec2(-999.0f, -999.0f);
	b2Vec2 normal = b2Vec2(-999.0f, -999.0f);
};

class ContactListener : public b2ContactListener {
public:
	void BeginContact(b2Contact* contact) override {
		auto fixture_a = contact->GetFixtureA();
		auto fixture_b = contact->GetFixtureB();
		Collision collision_a;
		Collision collision_b;
		collision_b.other = reinterpret_cast<Actor*>(fixture_a->GetUserData().pointer);
		collision_a.other = reinterpret_cast<Actor*>(fixture_b->GetUserData().pointer);
		collision_a.relative_velocity = fixture_a->GetBody()->GetLinearVelocity() - fixture_b->GetBody()->GetLinearVelocity();
		collision_b.relative_velocity = collision_a.relative_velocity;
		if (fixture_a->GetFilterData().categoryBits == COLLIDER_CATEGORY) {
			b2WorldManifold manifold;
			contact->GetWorldManifold(&manifold);
			collision_a.point = manifold.points[0];
			collision_b.point = manifold.points[0];
			collision_a.normal = manifold.normal;
			collision_b.normal = manifold.normal;

			for (auto component : collision_b.other->collisionEnterComponents) {
				try {
					(*component)["OnCollisionEnter"](*component, collision_a);
				}
				catch (const luabridge::LuaException& e) {
					ComponentDB::ReportError(collision_b.other->GetName(), e);
				}
			}
			for (auto component : collision_a.other->collisionEnterComponents) {
				try {
					(*component)["OnCollisionEnter"](*component, collision_b);
				}
				catch (const luabridge::LuaException& e) {
					ComponentDB::ReportError(collision_a.other->GetName(), e);
				}
			}
		}
		else {
			for (auto component : collision_b.other->triggerEnterComponents) {
				try {
					(*component)["OnTriggerEnter"](*component, collision_a);
				}
				catch (const luabridge::LuaException& e) {
					ComponentDB::ReportError(collision_b.other->GetName(), e);
				}
			}
			for (auto component : collision_a.other->triggerEnterComponents) {
				try {
					(*component)["OnTriggerEnter"](*component, collision_b);
				}
				catch (const luabridge::LuaException& e) {
					ComponentDB::ReportError(collision_a.other->GetName(), e);
				}
			}
		}
	}
	void EndContact(b2Contact* contact) override {
		auto fixture_a = contact->GetFixtureA();
		auto fixture_b = contact->GetFixtureB();
		Collision collision_a;
		Collision collision_b;
		collision_b.other = reinterpret_cast<Actor*>(fixture_a->GetUserData().pointer);
		collision_a.other = reinterpret_cast<Actor*>(fixture_b->GetUserData().pointer);
		collision_a.relative_velocity = fixture_a->GetBody()->GetLinearVelocity() - fixture_b->GetBody()->GetLinearVelocity();
		collision_b.relative_velocity = collision_a.relative_velocity;

		if (fixture_a->GetFilterData().categoryBits == COLLIDER_CATEGORY) {
			for (auto component : collision_b.other->collisionExitComponents) {
				try {
					(*component)["OnCollisionExit"](*component, collision_a);
				}
				catch (const luabridge::LuaException& e) {
					ComponentDB::ReportError(collision_b.other->GetName(), e);
				}
			}
			for (auto component : collision_a.other->collisionExitComponents) {
				try {
					(*component)["OnCollisionExit"](*component, collision_b);
				}
				catch (const luabridge::LuaException& e) {
					ComponentDB::ReportError(collision_a.other->GetName(), e);
				}
			}
		}
		else {
			for (auto component : collision_b.other->triggerExitComponents) {
				try {
					(*component)["OnTriggerExit"](*component, collision_a);
				}
				catch (const luabridge::LuaException& e) {
					ComponentDB::ReportError(collision_b.other->GetName(), e);
				}
			}
			for (auto component : collision_a.other->triggerExitComponents) {
				try {
					(*component)["OnTriggerExit"](*component, collision_b);
				}
				catch (const luabridge::LuaException& e) {
					ComponentDB::ReportError(collision_a.other->GetName(), e);
				}
			}
		}
	}
};

struct HitResult {
	Actor* actor = nullptr;
	b2Vec2 point = b2Vec2(0.0f, 0.0f);
	b2Vec2 normal = b2Vec2(0.0f, 0.0f);
	bool is_trigger = false;
};

class RaycastCallbackSingle : public b2RayCastCallback {
public:
	RaycastCallbackSingle() {}
	HitResult result;

	float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override {
		if (fixture->GetFilterData().categoryBits == 0x0000) {
			return 1.0f;
		}
		else if (fixture->GetFilterData().categoryBits == COLLIDER_CATEGORY) {
			result.actor = reinterpret_cast<Actor*>(fixture->GetUserData().pointer);
			result.point = point;
			result.normal = normal;
			return fraction;
		}
		else if (fixture->GetFilterData().categoryBits == TRIGGER_CATEGORY) {
			result.actor = reinterpret_cast<Actor*>(fixture->GetUserData().pointer);
			result.point = point;
			result.normal = normal;
			result.is_trigger = true;
			return fraction;
		}
        else {
            return 1.0f;
        }
	}
};

class RaycastCallbackAll : public b2RayCastCallback {
public:
	RaycastCallbackAll() {}
	std::vector<std::pair<float, HitResult>> results;

	float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override {
		if (fixture->GetFilterData().categoryBits != 0x0000) {
			HitResult result;
			result.actor = reinterpret_cast<Actor*>(fixture->GetUserData().pointer);
			result.point = point;
			result.normal = normal;
			result.is_trigger = (fixture->GetFilterData().categoryBits == TRIGGER_CATEGORY);

			results.emplace_back(fraction, std::move(result));
			SortRes();
			return 1.0f;
		}
		return 1.0f;
	}


	void SortRes() {
		std::sort(results.begin(), results.end(),
			[](const std::pair<float, HitResult>& a, const std::pair<float, HitResult>& b) {
				return a.first < b.first;
			});
	}
private:
};
