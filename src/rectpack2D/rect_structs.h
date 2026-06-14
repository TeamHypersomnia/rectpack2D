#pragma once
#include <algorithm>

namespace rectpack2D {
	using total_area_type = int;

	struct rect_wh {
		rect_wh() : w(0), h(0) {}
		rect_wh(const int w_, const int h_) : w(w_), h(h_) {}

		int w;
		int h;

		auto& flip() {
			std::swap(w, h);
			return *this;
		}

		int max_side() const {
			return h > w ? h : w;
		}

		int min_side() const {
			return h < w ? h : w;
		}

		int	area() const { return w * h; }
	   	int perimeter() const { return 2 * w + 2 * h; }

		template <class R>
		void expand_with(const R& r) {
			w = std::max(w, r.x + r.w);
			h = std::max(h, r.y + r.h);
		}
	};

	struct rect_xywh {
		int x;
		int	y;
		int w;
		int h;

		rect_xywh() : x(0), y(0), w(0), h(0) {}
		rect_xywh(const int x_, const int y_, const int w_, const int h_) : x(x_), y(y_), w(w_), h(h_) {}

		int	area() const { return w * h; }
		int perimeter() const { return 2 * w + 2 * h; }

		auto get_wh() const {
			return rect_wh(w, h);
		}

		auto& get_rect() {
			return *this;
		}

		const auto& get_rect() const {
			return *this;
		}
	};

	struct rect_xywhf {
		int x;
		int y;
		int w;
		int h;
		bool flipped;

		rect_xywhf() : x(0), y(0), w(0), h(0), flipped(false) {}
		rect_xywhf(const int x_, const int y_, const int w_, const int h_, const bool flipped_) : x(x_), y(y_), w(flipped_ ? h_ : w_), h(flipped_ ? w_ : h_), flipped(flipped_) {}
		rect_xywhf(const rect_xywh& b) : rect_xywhf(b.x, b.y, b.w, b.h, false) {}

		int	area() const { return w * h; }
		int perimeter() const { return 2 * w + 2 * h; }

		auto get_wh() const {
			return rect_wh(w, h);
		}

		auto& get_rect() {
			return *this;
		}

		const auto& get_rect() const {
			return *this;
		}
	};

	using space_rect = rect_xywh;
}
