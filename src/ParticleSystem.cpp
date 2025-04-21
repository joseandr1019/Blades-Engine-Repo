#include "ComponentDB.h"
#include "ImageDB.h"
#include "ParticleSystem.h"
#include "Renderer.h"

#include <iostream>
#include <limits>

#include "box2d/box2d.h"
#include "glm/glm.hpp"
#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"

std::string ReturnParticleSystemType() {
	return "ParticleSystem";
}

void ParticleSystem::LuaInit() {
	luabridge::getGlobalNamespace(ComponentDB::GetLuaState())
		.beginClass<ParticleSystem>("ParticleSystem")
		.addProperty("enabled", &ParticleSystem::enabled)
		.addProperty("removed", &ParticleSystem::removed)
		.addProperty("frames_between_bursts", &ParticleSystem::frames_between_bursts)
		.addProperty("burst_quantity", &ParticleSystem::burst_quantity)
		.addProperty("duration_frames", &ParticleSystem::duration_frames)
		.addProperty("sorting_order", &ParticleSystem::sorting_order)
		.addProperty("start_color_r", &ParticleSystem::start_color_r)
		.addProperty("start_color_b", &ParticleSystem::start_color_b)
		.addProperty("start_color_g", &ParticleSystem::start_color_g)
		.addProperty("start_color_a", &ParticleSystem::start_color_a)
		.addProperty("end_color_r", &ParticleSystem::end_color_r)
		.addProperty("end_color_g", &ParticleSystem::end_color_g)
		.addProperty("end_color_b", &ParticleSystem::end_color_b)
		.addProperty("end_color_a", &ParticleSystem::end_color_a)
		.addProperty("x", &ParticleSystem::x)
		.addProperty("y", &ParticleSystem::y)
		.addProperty("emit_angle_min", &ParticleSystem::emit_angle_min)
		.addProperty("emit_angle_max", &ParticleSystem::emit_angle_max)
		.addProperty("emit_radius_min", &ParticleSystem::emit_radius_min)
		.addProperty("emit_radius_max", &ParticleSystem::emit_radius_max)
		.addProperty("start_scale_min", &ParticleSystem::start_scale_min)
		.addProperty("start_scale_max", &ParticleSystem::start_scale_max)
		.addProperty("end_scale", &ParticleSystem::end_scale)
		.addProperty("start_speed_min", &ParticleSystem::start_speed_min)
		.addProperty("start_speed_max", &ParticleSystem::start_speed_max)
		.addProperty("gravity_scale_x", &ParticleSystem::gravity_scale_x)
		.addProperty("gravity_scale_y", &ParticleSystem::gravity_scale_y)
		.addProperty("rotation_min", &ParticleSystem::rotation_min)
		.addProperty("rotation_max", &ParticleSystem::rotation_max)
		.addProperty("rotation_speed_min", &ParticleSystem::rotation_speed_min)
		.addProperty("rotation_speed_max", &ParticleSystem::rotation_speed_max)
		.addProperty("drag_factor", &ParticleSystem::drag_factor)
		.addProperty("angular_drag_factor", &ParticleSystem::angular_drag_factor)
		.addProperty("actor", &ParticleSystem::actor)
		.addProperty("image", &ParticleSystem::image)
		.addProperty("key", &ParticleSystem::key)
		.addProperty("type", &ParticleSystem::type)
		.addFunction("OnStart", &ParticleSystem::OnStart)
		.addFunction("OnUpdate", &ParticleSystem::OnUpdate)
		.addFunction("Stop", &ParticleSystem::Stop)
		.addFunction("Play", &ParticleSystem::Play)
		.addFunction("Burst", &ParticleSystem::Burst)
		.addStaticFunction("getType", &ReturnParticleSystemType)
		.endClass();
}

void ParticleSystem::OnStart() {
	if (this->image == "") {
		ImageDB::CreateDefaultTextureWithName("default");
		this->image = "default";
	}
	else {
		SDL_Texture* _;
		ImageDB::LoadViewImage(Renderer::renderer_ptr, this->image, _);
	}

	this->emit_angle_distribution = RandomEngine(this->emit_angle_min, this->emit_angle_max, 298);
	this->emit_radius_distribution = RandomEngine(this->emit_radius_min, this->emit_radius_max, 404);

	this->scale_distribution = RandomEngine(this->start_scale_min, this->start_scale_max, 494);
	this->rotation_distribution = RandomEngine(this->rotation_min, this->rotation_max, 440);

	this->speed_distribution = RandomEngine(this->start_speed_min, this->start_speed_max, 498);
	this->omega_distribution = RandomEngine(this->rotation_speed_min, this->rotation_speed_max, 305);
}

void ParticleSystem::Burst() {
	if (this->burst_quantity < 1) {
		this->burst_quantity = 1;
	}

	FixColors();

	for (int q = 0; q < this->burst_quantity; q++) {
		float angle_radians = glm::radians(this->emit_angle_distribution.Sample());
		float radius = this->emit_radius_distribution.Sample();

		float cos_angle = glm::cos(angle_radians);
		float sin_angle = glm::sin(angle_radians);

		float scale = this->scale_distribution.Sample();
		float rotation = this->rotation_distribution.Sample();

		float vel = this->speed_distribution.Sample();
		float omega = this->omega_distribution.Sample();

		if (this->free_indexes.empty()) {
			this->particle_active.emplace_back(true);
			this->particle_spawn_frame.emplace_back(this->local_frame);
			this->particle_color.emplace_back(Color(this->start_color_r, this->start_color_g, this->start_color_b, this->start_color_a));

			this->particle_position.emplace_back(Pos(this->x + cos_angle * radius, this->y + sin_angle * radius));
			this->particle_physics.emplace_back(Physics(vel * cos_angle, vel * sin_angle, this->gravity_scale_x, this->gravity_scale_y, this->drag_factor, omega, this->angular_drag_factor));
			this->particle_scale.emplace_back(Scale(rotation, 0.5f, 0.5f, scale));
		}
		else {
			int index = this->free_indexes.front();
			this->free_indexes.pop_front();

			this->particle_active[index] = true;
			this->particle_spawn_frame[index] = this->local_frame;

			this->particle_position[index] = Pos(this->x + cos_angle * radius, this->y + sin_angle * radius);
			this->particle_physics[index] = Physics(vel * cos_angle, vel * sin_angle, this->gravity_scale_x, this->gravity_scale_y, this->drag_factor, omega, this->angular_drag_factor);
			this->particle_scale[index] = Scale(rotation, 0.5f, 0.5f, scale);
		}
	}
}

void ParticleSystem::OnUpdate() {
	if (this->frames_between_bursts < 1) {
		this->frames_between_bursts = 1;
	}

	if (this->duration_frames < 1) {
		this->duration_frames = 1;
	}

	if (this->local_frame % this->frames_between_bursts == 0
		&& !this->stopped) {
		this->Burst();
	}

	for (int i = 0; i < this->particle_active.size(); i++) {
		if (this->particle_active[i]) {
			if (this->local_frame - this->particle_spawn_frame[i] < this->duration_frames) {
				float scales = this->particle_scale[i].scale;
				float lifetime_progress = static_cast<float>(this->local_frame - this->particle_spawn_frame[i]) / this->duration_frames;
				int red = this->particle_color[i].r;
				int green = this->particle_color[i].g;
				int blue = this->particle_color[i].b;
				int alpha = this->particle_color[i].a;
				if (this->end_scale != std::numeric_limits<float>::min()) {
					scales = glm::mix(scales, this->end_scale, lifetime_progress);
				}
				if (this->end_color_r != -256) {
					red = glm::mix(red, this->end_color_r, lifetime_progress);
				}
				if (this->end_color_g != -256) {
					green = glm::mix(green, this->end_color_g, lifetime_progress);
				}
				if (this->end_color_b != -256) {
					blue = glm::mix(blue, this->end_color_g, lifetime_progress);
				}
				if (this->end_color_a != -256) {
					alpha = glm::mix(alpha, this->end_color_a, lifetime_progress);
				}

				this->particle_physics[i].x_vel += this->particle_physics[i].x_grav;
				this->particle_physics[i].y_vel += this->particle_physics[i].y_grav;
				this->particle_physics[i].x_vel *= this->particle_physics[i].drag;
				this->particle_physics[i].y_vel *= this->particle_physics[i].drag;
				this->particle_physics[i].omega *= this->particle_physics[i].angular_drag;
				this->particle_position[i].x += this->particle_physics[i].x_vel;
				this->particle_position[i].y += this->particle_physics[i].y_vel;
				this->particle_scale[i].rotation += this->particle_physics[i].omega;

				

				ImageDB::DrawEx(this->image, this->particle_position[i].x, this->particle_position[i].y,
					this->particle_scale[i].rotation, scales, scales,
					0.5f, 0.5f, red, green, blue, alpha, this->sorting_order);
			}
			else {
				this->particle_active[i] = false;
				this->free_indexes.push_back(i);
			}
		}
	}

	this->local_frame++;
}