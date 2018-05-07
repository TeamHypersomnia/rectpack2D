#pragma once
#include <utility>

namespace rectpack2D {
	using total_area_type = int;

	template <class F, class G>
	struct finder_input {
		const int max_bin_side;
		const int discard_step;
		F handle_successful_insertion;
		G handle_unsuccessful_insertion;
	};

	template <class F, class G>
	auto make_finder_input(
		const int max_bin_side,
		const int discard_step,
		F&& handle_successful_insertion,
		G&& handle_unsuccessful_insertion
	) {
		return finder_input<F, G> { 
			max_bin_side, discard_step, std::move(handle_successful_insertion), std::move(handle_unsuccessful_insertion)
		};
	};

	struct rect_wh {
		rect_wh() : w(0), h(0) {}
		rect_wh(const int w, const int h) : w(w), h(h) {}

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
			return h > w ? h : w;
		}

		int	area() const { return w * h; }
	   	int perimeter() const { return 2 * w + 2 * h; }

		int pathological_mult() const {
			return float(max_side()) / min_side() * area();
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

	using space_rect = rect_xywh;
}