#pragma once

/* 
	This could be templatized a lot but this would be splitting hairs a lot,
	and repetition sometimes communicates intent better.
*/

namespace rectpack {
	struct rect_wh {
		rect_wh() : w(0), h(0) {}
		rect_wh(const int w, const int h) : w(w), h(h) {}

		int w;
		int h;

		auto& flip() {
			std::swap(w, h);
			return *this;
		}

		int	area() const { return w * h; }
	   	int perimeter() const { return 2 * w + 2 * h; }

		bool max_side_greater(const rect_wh b) const {
			return std::max(w, h) > std::max(b.w, b.h);
		}
	};

	struct rect_xywh {
		int x;
		int	y;
		int w;
		int h;

		rect_xywh() : x(0), y(0), w(0), h(0) {}
		rect_xywh(const int x, const int y, const int w, const int h) : x(x), y(y), w(w), h(h) {}

		int	area() const { return w * h; }
		int perimeter() const { return 2 * w + 2 * h; }

		auto get_wh() const {
			return rect_wh(w, h);
		}
	};

	struct rect_xywhf {
		int x;
		int y;
		int w;
		int h;
		bool flipped;

		rect_xywhf() : x(0), y(0), w(0), h(0), flipped(false) {}
		rect_xywhf(const int x, const int y, const int w, const int h, const bool flipped) : x(x), y(y), w(flipped ? h : w), h(flipped ? w : h), flipped(flipped) {}
		rect_xywhf(const rect_xywh& b) : rect_xywhf(b.x, b.y, b.w, b.h, false) {}

		int	area() const { return w * h; }
		int perimeter() const { return 2 * w + 2 * h; }

		auto get_wh() const {
			return rect_wh(w, h);
		}
	};
}