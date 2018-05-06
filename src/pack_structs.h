#include "pack.h"
#include <cstring>
#include <algorithm>

namespace rectpack {
	struct rect_xywh;

	struct rect_wh {
		rect_wh(const rect_xywh&);
		rect_wh(int w = 0, int h = 0);

		int w;
		int h;

		auto& flip() {
			std::swap(w, h);
			return *this;
		}

		int	area() const;
	   	int perimeter() const;
	};

	struct rect_xywh : public rect_wh {
		rect_xywh();
		rect_xywh(int x, int y, int width, int height);

		int x;
		int	y;
	};

	struct rect_xywhf : public rect_xywh {
		rect_xywhf(int x, int y, int width, int height);
		rect_xywhf();
		void flip();
		bool flipped;
	};

	rect_wh::rect_wh(const rect_xywh& rr) : w(rr.w), h(rr.h) {} 
	rect_wh::rect_wh(int w, int h) : w(w), h(h) {}

	rect_xywh::rect_xywh() : x(0), y(0) {}
	rect_xywh::rect_xywh(int x, int y, int w, int h) : rect_wh(w, h), x(x), y(y) {}

	int rect_wh::area() const {
		return w*h;
	}

	int rect_wh::perimeter() const {
		return 2*w + 2*h; 
	}

	rect_xywhf::rect_xywhf(int x, int y, int width, int height) : rect_xywh(x, y, width, height), flipped(false) {}
	rect_xywhf::rect_xywhf() : flipped(false) {}
}