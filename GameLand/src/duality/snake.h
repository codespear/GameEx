/**
 * Copyright 2012 Willem Duminy
 * See LICENCE.txt
 */

#pragma once
#include "../game/game.h"
#include "../game/game_math.h"
#include "../game/collisions2d.h"
#include <array>


namespace duality {
	using namespace game;

	const size_t SNAKE_MEM_SIZE = 1000;
	const Scalar SNAKE_FLOOR = 0.0f;
	const Scalar SNAKE_WIDTH_HALF = 0.07f;
	const Scalar SNAKE_WIDTH = SNAKE_WIDTH_HALF * 2.0f;

	enum SteerDirection : int {
		Left,
		Forward,
		Right
	};

	class SpinePoint {
		PREVENT_COPY(SpinePoint);
	public:
		SpinePoint();
		void assign(const Vector& topMiddlePoint);
		void assign(const Vector& topMiddlePoint, const SpinePoint* previousPoint);
		const Vector& topMiddle() const {return _topMiddle;}
		const Vector& bottomLeft() const {return _bottomLeft;}
		const Vector& bottomRight() const {return _bottomRight;}
		CollidablePolygon & polygon() const;
	private:
		Vector _topMiddle;
		Vector _bottomLeft;
		Vector _bottomRight;
		mutable CollidablePolygon _polygon;
		mutable bool _poly_is_dirty;
		SpinePoint * _previous;
	public:
		typedef std::unique_ptr<SpinePoint> u_ptr;
	};

	typedef std::array<SpinePoint,SNAKE_MEM_SIZE> SnakeArray;

	class Snake {
	public:
		Snake(const Vector& startingPoint = Vector::origin + (Vector::up * (SNAKE_WIDTH_HALF)),
				const Vector& lookingAt = Vector::north,
				const int updatesPerSegment = 5,
				const Scalar& distancePerMove = 0.02,
				const Scalar& radiansPerTurn = degrees45 / 10.0f,
				const int initialGrowth = 200);
		void move(const SteerDirection& steerDir = Forward);
		size_t size() const {return (_tail-_head);};
		void grow(const unsigned int sizeIncrement);
		bool is_growing() const {return _remaining_growth;}
		const SpinePoint& head() const {return _points[_head];};
		const SpinePoint& tail() const {return _points[_tail];};
		virtual ~Snake(){};
		void kill() {_is_alive = false;}
	protected:
		const RotateInt& head_index() const {return _head;};
		const RotateInt& tail_index() const {return _tail;};
		const SnakeArray& points() const {return _points;}
		virtual void on_head_added(){};
		virtual void before_tail_remove(){};
	private:
		SnakeArray _points;
		RotateInt _head, _tail;
		Vector _move_vector;
		Scalar _rotation_angle;
		const Scalar _radians_per_move;
		const Vector _translation_vector;
		unsigned int _remaining_growth;
		CountDownCounter _time_to_grow;
		int _previous_head;
		bool _is_alive;
	};

	class SnakeObject : public GameObject, private Snake {
	public:
		SnakeObject(CollisionManager &mgr);
		void initialise(const ResourceContext &ctx, const DrawContext &draw) override;
		void draw(const DrawContext& gc) override;
		void update(const UpdateContext & uc) override;
		virtual ~SnakeObject(){}
	protected:
		void on_head_added() override;
		void before_tail_remove() override;
		bool _left_key_down, _right_key_down;
		ShaderProgram::u_ptr _program_p;
		Texture::u_ptr _tex_p;
		CollisionManager& _col_mgr;
	};
}