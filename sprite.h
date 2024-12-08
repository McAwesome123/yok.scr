#pragma once

#include <array>
#include <algorithm>
#include <iterator>
#include <tuple>
#include <utility>
#include <map>
#include <vector>
#include <set>
#include <numeric>
#include <chrono>

#include "context.h"
#include "graphics.h"

class TrailSprite;

class Sprite : public Identifiable<Empty> {
public:
	Sprite(const Texture *texture, const Point &home, const bool has_trail = true);

	void change_texture(const Texture *texture);
	virtual void draw(Context &ctx);
	virtual void update(Context &ctx);

	template <int C> double final() const {
		return std::get<C>(m_home) + std::get<C>(m_relpos);
	}

	Point &home();

protected:
	static double wrap(double home, double total, double min, double max);
	static Point get_bounds_correction(Context &ctx);
	Point get_screen_min_bounds(Context &ctx);
	Point get_screen_max_bounds(Context &ctx);

	void transform();
	void update_trail(Context &ctx);
	void increment_trail_index(const size_t amount = 1);
	TrailSprite& get_trail(const size_t index = 0);
	virtual void draw_trail(Context &ctx);

	const Texture *m_texture;
	Point m_relpos;
	Point m_home;
	GLdouble m_size;

	std::vector<TrailSprite> m_trail;
	size_t m_trail_start_index;

	double m_edge_boundary;
};

class Yonker : public Sprite {
public:
	enum Emotion {
		OPTIMISM = 0,
		EMPATHY = 1,
		AMBITION = 2,
		_EMOTIONS_COUNT
	};
	using EmotionVector = std::array<double, _EMOTIONS_COUNT>;

	Yonker(const Texture *texture, const Point &home);

	virtual void update(Context &ctx) override;

protected:
	const BitmapData &bitmap_for_current_emotion(Context &ctx) const;
	EmotionVector emotion_vector(Context &ctx) const;

	EmotionVector m_emotion_vector;
};

class Impostor : public Sprite {
public:
	Impostor(const PaletteData *palette, const Point &home);

	virtual void update(Context &ctx) override;

protected:
	static Bitmaps::Definition &random_bitmap();
};

class TrailSprite : public Sprite {
public:
	TrailSprite(const Texture *palette, const Point &home);

	virtual void update(Context &ctx) override;

	static size_t get_trail_length();
	static double get_trail_space();

protected:
	virtual void draw_trail(Context &ctx) override;
};
