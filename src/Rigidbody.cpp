#include "ComponentDB.h"
#include "Rigidbody.h"
#include "SceneDB.h"

#include "box2d/box2d.h"

void Rigidbody::OnStart() {
	b2BodyDef bdef;
	bdef.position.x = this->x;
	bdef.position.y = this->y;
	if (this->body_type == "static") {
		bdef.type = b2_staticBody;
	}
	else if (this->body_type == "kinematic") {
		bdef.type = b2_kinematicBody;
	}
	else {
		bdef.type = b2_dynamicBody;
	}
	bdef.bullet = this->precise;
	bdef.gravityScale = this->gravity_scale;
	bdef.angularDamping = this->angular_friction;
	bdef.angle = this->rotation * (b2_pi / 180.0f);

	this->body = SceneDB::world.CreateBody(&bdef);

	if (!this->has_collider && !this->has_trigger) {
		b2PolygonShape phantom_shape;
		phantom_shape.SetAsBox(this->width * 0.5f, this->height * 0.5f);

		b2FixtureDef phantom_fixture_def;
		b2Filter phantomFilter;
		phantomFilter.categoryBits = 0x0000;
		phantomFilter.maskBits = 0x0000;
		phantom_fixture_def.filter = phantomFilter;
		phantom_fixture_def.shape = &phantom_shape;
		phantom_fixture_def.density = this->density;
		phantom_fixture_def.friction = this->friction;
		phantom_fixture_def.restitution = this->bounciness;

		phantom_fixture_def.isSensor = true;
		phantom_fixture_def.userData.pointer = reinterpret_cast<uintptr_t>(this->actor);
		this->body->CreateFixture(&phantom_fixture_def);
	}
	else {
		if (this->has_collider) {
			b2FixtureDef collider_fixture_def;
			b2Filter collider_filter;
			collider_filter.categoryBits = COLLIDER_CATEGORY;
			collider_filter.maskBits = COLLIDER_CATEGORY;
			collider_fixture_def.filter = collider_filter;
			collider_fixture_def.density = this->density;
			collider_fixture_def.friction = this->friction;
			collider_fixture_def.restitution = this->bounciness;
			collider_fixture_def.userData.pointer = reinterpret_cast<uintptr_t>(this->actor);
			collider_fixture_def.isSensor = false;

			if (this->collider_type == "circle") {
				b2CircleShape circle_shape;
				circle_shape.m_radius = this->radius;
				collider_fixture_def.shape = &circle_shape;
				this->body->CreateFixture(&collider_fixture_def);
			}
			else {
				b2PolygonShape box_shape;
				box_shape.SetAsBox(this->width * 0.5f, this->height * 0.5f);
				collider_fixture_def.shape = &box_shape;
				this->body->CreateFixture(&collider_fixture_def);
			}
		}

		if (this->has_trigger) {
			b2FixtureDef trigger_fixture_def;
			b2Filter trigger_filter;
			trigger_filter.categoryBits = TRIGGER_CATEGORY;
			trigger_filter.maskBits = TRIGGER_CATEGORY;
			trigger_fixture_def.filter = trigger_filter;

			trigger_fixture_def.density = this->density;
			trigger_fixture_def.friction = this->friction;
			trigger_fixture_def.restitution = this->bounciness;
			trigger_fixture_def.userData.pointer = reinterpret_cast<uintptr_t>(this->actor);
			trigger_fixture_def.isSensor = true;

			if (this->trigger_type == "circle") {
				b2CircleShape circle_shape;
				circle_shape.m_radius = this->trigger_radius;
				trigger_fixture_def.shape = &circle_shape;
				this->body->CreateFixture(&trigger_fixture_def);
			}
			else {
				b2PolygonShape box_shape;
				box_shape.SetAsBox(this->trigger_width * 0.5f, this->trigger_height * 0.5f);
				trigger_fixture_def.shape = &box_shape;
				this->body->CreateFixture(&trigger_fixture_def);
			}
		}
	}

	ContactListener* listener = new ContactListener;
	SceneDB::world.SetContactListener(listener);
}

luabridge::LuaRef Raycast(b2Vec2 pos, b2Vec2 dir, float dist) {
	b2Vec2 end = pos + (dist * dir);
	RaycastCallbackSingle callback;
	SceneDB::world.RayCast(&callback, pos, end);
	if (callback.result.actor) {
		return luabridge::LuaRef(ComponentDB::GetLuaState(), callback.result);
	}
	else {
		return luabridge::LuaRef(ComponentDB::GetLuaState());
	}
}

luabridge::LuaRef RaycastAll(b2Vec2 pos, b2Vec2 dir, float dist) {
	luabridge::LuaRef actorTable = luabridge::newTable(ComponentDB::GetLuaState());
	b2Vec2 end = pos + (dist * dir);
	RaycastCallbackAll callback;
	SceneDB::world.RayCast(&callback, pos, end);
	callback.SortRes();
	int iter = 1;
	for (const auto& result : callback.results) {
		if (result.second.actor) {
			actorTable[iter] = result.second;
			iter++;
		}
	}
	return actorTable;
}

std::string ReturnRigidbodyType() {
	return "Rigidbody";
}

void Rigidbody::LuaInit() {
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginClass<Rigidbody>("Rigidbody")
		.addConstructor<void(*)()>()
		.addProperty("precise", &Rigidbody::precise)
		.addProperty("has_collider", &Rigidbody::has_collider)
		.addProperty("has_trigger", &Rigidbody::has_trigger)
		.addProperty("enabled", &Rigidbody::enabled)
		.addProperty("removed", &Rigidbody::removed)
		.addProperty("x", &Rigidbody::x)
		.addProperty("y", &Rigidbody::y)
		.addProperty("gravity_scale", &Rigidbody::gravity_scale)
		.addProperty("density", &Rigidbody::density)
		.addProperty("angular_friction", &Rigidbody::angular_friction)
		.addProperty("rotation", &Rigidbody::rotation)
		.addProperty("width", &Rigidbody::width)
		.addProperty("height", &Rigidbody::height)
		.addProperty("radius", &Rigidbody::radius)
		.addProperty("trigger_width", &Rigidbody::trigger_width)
		.addProperty("trigger_height", &Rigidbody::trigger_height)
		.addProperty("trigger_radius", &Rigidbody::trigger_radius)
		.addProperty("friction", &Rigidbody::friction)
		.addProperty("bounciness", &Rigidbody::bounciness)
		.addProperty("body", &Rigidbody::body)
		.addProperty("actor", &Rigidbody::actor)
		.addProperty("body_type", &Rigidbody::body_type)
		.addProperty("collider_type", &Rigidbody::collider_type)
		.addProperty("trigger_type", &Rigidbody::trigger_type)
		.addProperty("key", &Rigidbody::key)
		.addProperty("type", &Rigidbody::type)
		.addFunction("GetPosition", &Rigidbody::GetPosition)
		.addFunction("GetRotation", &Rigidbody::GetRotation)
		.addFunction("OnStart", &Rigidbody::OnStart)
		.addFunction("OnDestroy", &Rigidbody::OnDestroy)
		.addFunction("AddForce", &Rigidbody::AddForce)
		.addFunction("SetVelocity", &Rigidbody::SetVelocity)
		.addFunction("SetPosition", &Rigidbody::SetPosition)
		.addFunction("SetRotation", &Rigidbody::SetRotation)
		.addFunction("SetAngularVelocity", &Rigidbody::SetAngularVelocity)
		.addFunction("SetGravityScale", &Rigidbody::SetGravityScale)
		.addFunction("SetUpDirection", &Rigidbody::SetUpDirection)
		.addFunction("SetRightDirection", &Rigidbody::SetRightDirection)
		.addFunction("GetVelocity", &Rigidbody::GetVelocity)
		.addFunction("GetAngularVelocity", &Rigidbody::GetAngularVelocity)
		.addFunction("GetGravityScale", &Rigidbody::GetGravityScale)
		.addFunction("GetUpDirection", &Rigidbody::GetUpDirection)
		.addFunction("GetRightDirection", &Rigidbody::GetRightDirection)
		.addStaticFunction("getType", &ReturnRigidbodyType)
		.endClass();
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginClass<Collision>("Collision")
		.addProperty("other", &Collision::other)
		.addProperty("point", &Collision::point)
		.addProperty("relative_velocity", &Collision::relative_velocity)
		.addProperty("normal", &Collision::normal)
		.endClass();
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginClass<HitResult>("HitResult")
		.addProperty("actor", &HitResult::actor)
		.addProperty("point", &HitResult::point)
		.addProperty("normal", &HitResult::normal)
		.addProperty("is_trigger", &HitResult::is_trigger)
		.endClass();
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginNamespace("Physics")
		.addFunction("Raycast", &Raycast)
		.addFunction("RaycastAll", &RaycastAll)
		.endNamespace();
}
