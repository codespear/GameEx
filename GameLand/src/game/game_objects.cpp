/**
 * Copyright 2012 Willem Duminy
 * See LICENCE.txt
 */
#include "game_objects.h"

namespace game {

	TriangleStrip::TriangleStrip(const int numberOfTriangles) :
		_verts(new GLfloat[(numberOfTriangles+2)*3]),
		_ins(_verts.get()),
		_end(_verts.get() + (numberOfTriangles+2)*3),
		_it(_ins)
	{}
	void TriangleStrip::push_back(const GLfloat& x, const GLfloat& y, const GLfloat& z) {
		if (_ins >= _end)
			throw std::runtime_error("strip past is capacity");
		*(_ins++) = x;
		*(_ins++) = y;
		*(_ins++) = z;
	}

	void TriangleStrip::push_back3f(const GLfloat* xyz) {
		push_back(xyz[0],xyz[1],xyz[2]);
	}

	void TriangleStrip::draw() const {
		reset();
		while (!at_end())
			draw_step();
	}

	SphereCamera::SphereCamera(int drawOrder, double y, double r)
	: GameObject(drawOrder), _circleY(y), _circleRadius(r), _theta(0), _delta(0) {}

	void SphereCamera::update(const UpdateContext& ctx) {
		const static double a360 = 3.14 * 2.0;
		const static double degrees_per_second = 90;
		const static double rads_per_second = a360 * degrees_per_second / 360.0;
		const static double rads_per_update = ctx.seconds_per_update() * rads_per_second;
		switch (ctx.key_down()) {
			case SDLK_LEFT:
				_delta = -rads_per_update;
				break;
			case SDLK_RIGHT:
				_delta = +rads_per_update;
				break;
			default:
				break;
		}
		switch (ctx.key_up()) {
			case SDLK_LEFT:
			case SDLK_RIGHT:
				_delta = 0;
				break;
			default:
				break;
		}
		_theta += _delta;
	}

	void SphereCamera::draw(const DrawContext&) {
		gluLookAt(_circleRadius * sin(_theta),
				       _circleY,
				       _circleRadius * cos(_theta),
				       0,0,0,0,1,0);
	}

}



