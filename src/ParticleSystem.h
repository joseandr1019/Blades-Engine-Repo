#pragma once
#include "Helper.h"

#include <limits>
#include <string>
#include <vector>

struct Color {
	int r = 255;
	int g = 255;
	int b = 255;
	int a = 255;

	Color(int r_init, int g_init, int b_init, int a_init) {
		r = r_init;
		g = g_init;
		b = b_init;
		a = a_init;
	}
};

struct Pos {
	float x = 0.0f;
	float y = 0.0f;

	Pos(float init_x, float init_y) {
		x = init_x;
		y = init_y;
	}
};

struct Scale {
	float rotation = 0.0f;
	float x_piv = 0.5f;
	float y_piv = 0.5f;
	float scale = 1.0f;

	Scale(float rot, float piv_x, float piv_y, float sca) {
		rotation = rot;
		x_piv = piv_x;
		y_piv = piv_y;
		scale = sca;
	}
};

struct Physics {
	float x_vel = 0.0f;
	float y_vel = 0.0f;
	float x_grav = 0.0f;
	float y_grav = 0.0f;
	float drag = 1.0f;
	float angular_drag = 1.0f;
	float omega = 0.0f;

	Physics(float new_x_vel, float new_y_vel, float new_x_grav, float new_y_grav, float new_drag, float new_omega, float new_angular_drag) {
		x_vel = new_x_vel;
		y_vel = new_y_vel;
		x_grav = new_x_grav;
		y_grav = new_y_grav;
		drag = new_drag;
		omega = new_omega;
		angular_drag = new_angular_drag;
	}
};

class ParticleSystem {
public:
	bool enabled = true;
	bool removed = false;
	int frames_between_bursts = 1;
	int burst_quantity = 1;
	int duration_frames = 300;
	int sorting_order = 9999;
	int start_color_r = 255;
	int start_color_g = 255;
	int start_color_b = 255;
	int start_color_a = 255;
	int end_color_r = -256;
	int end_color_g = -256;
	int end_color_b = -256;
	int end_color_a = -256;
	float x = 0.0f;
	float y = 0.0f;
	float emit_angle_min = 0.0f;
	float emit_angle_max = 360.0f;
	float emit_radius_min = 0.0f;
	float emit_radius_max = 0.5f;
	float start_scale_min = 1.0f;
	float start_scale_max = 1.0f;
	float end_scale = std::numeric_limits<float>::min();
	float rotation_min = 0.0f;
	float rotation_max = 0.0f;
	float start_speed_min = 0.0f;
	float start_speed_max = 0.0f;
	float rotation_speed_min = 0.0f;
	float rotation_speed_max = 0.0f;
	float gravity_scale_x = 0.0f;
	float gravity_scale_y = 0.0f;
	float drag_factor = 1.0f;
	float angular_drag_factor = 1.0f;
	Actor* actor = nullptr;
	std::string image = "";
	std::string key = "";
	std::string type = "ParticleSystem";

	ParticleSystem() {};
	ParticleSystem(const ParticleSystem* ps) {
		this->frames_between_bursts = ps->frames_between_bursts;
		this->burst_quantity = ps->burst_quantity;
		this->duration_frames = ps->duration_frames;
		this->sorting_order = ps->sorting_order;
		this->start_color_r = ps->start_color_r;
		this->start_color_g = ps->start_color_g;
		this->start_color_b = ps->start_color_b;
		this->start_color_a = ps->start_color_a;
		this->end_color_r = ps->end_color_r;
		this->end_color_g = ps->end_color_g;
		this->end_color_b = ps->end_color_b;
		this->end_color_a = ps->end_color_a;
		this->x = ps->x;
		this->y = ps->y;
		this->emit_angle_min = ps->emit_angle_min;
		this->emit_angle_max = ps->emit_angle_max;
		this->emit_radius_min = ps->emit_radius_min;
		this->emit_radius_max = ps->emit_radius_max;
		this->start_scale_min = ps->start_scale_min;
		this->start_scale_max = ps->start_scale_max;
		this->end_scale = ps->end_scale;
		this->rotation_min = ps->rotation_min;
		this->rotation_max = ps->rotation_max;
		this->start_speed_min = ps->start_speed_min;
		this->start_speed_max = ps->start_speed_max;
		this->rotation_speed_min = ps->rotation_speed_min;
		this->rotation_speed_max = ps->rotation_speed_max;
		this->gravity_scale_x = ps->gravity_scale_x;
		this->gravity_scale_y = ps->gravity_scale_y;
		this->drag_factor = ps->drag_factor;
		this->angular_drag_factor = ps->angular_drag_factor;
		this->key = ps->key;
		this->image = ps->image;
	}

	static void LuaInit();

	void OnStart();
	void OnUpdate();

	void Stop() {
		stopped = true;
	}

	void Play() {
		stopped = false;
	}

	void Burst();
private:
	bool stopped = false;
	int local_frame = 0;

	RandomEngine emit_angle_distribution;
	RandomEngine emit_radius_distribution;
	RandomEngine scale_distribution;
	RandomEngine rotation_distribution;
	RandomEngine speed_distribution;
	RandomEngine omega_distribution;

	std::vector<bool> particle_active;
	std::vector<int> particle_spawn_frame;
	std::vector<Pos> particle_position;
	std::vector<Color> particle_color;
	std::vector<Physics> particle_physics;
	std::vector<Scale> particle_scale;
	std::deque<int> free_indexes;

	void FixColors() {
		if (start_color_r < 0) {
			start_color_r = 0;
		}
		else if (start_color_r > 255) {
			start_color_r = 255;
		}

		if (start_color_g < 0) {
			start_color_g = 0;
		}
		else if (start_color_g > 255) {
			start_color_g = 255;
		}

		if (start_color_b < 0) {
			start_color_b = 0;
		}
		else if (start_color_b > 255) {
			start_color_b = 255;
		}

		if (start_color_a < 0) {
			start_color_a = 0;
		}
		else if (start_color_a > 255) {
			start_color_a = 255;
		}

		if (end_color_r != -256) {
			if (end_color_r < 0) {
				end_color_r = 0;
			}
			else if (end_color_r > 255) {
				end_color_r = 255;
			}
		}

		if (end_color_g != -256) {
			if (end_color_g < 0) {
				end_color_g = 0;
			}
			else if (end_color_g > 255) {
				end_color_g = 255;
			}
		}

		if (end_color_b != -256) {
			if (end_color_b < 0) {
				end_color_b = 0;
			}
			else if (end_color_b > 255) {
				end_color_b = 255;
			}
		}

		if (end_color_a != -256) {
			if (end_color_a < 0) {
				end_color_a = 0;
			}
			else if (end_color_a > 255) {
				end_color_a = 255;
			}
		}
	}
};