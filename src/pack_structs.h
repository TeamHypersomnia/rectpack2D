#include "pack.h"
#include <cstring>
#include <algorithm>

struct rect_ltrb;
struct rect_xywh;

enum class rect_wh_fitting {
	TOO_BIG,
	FITS_INSIDE,
	FITS_INSIDE_BUT_FLIPPED,
	FITS_EXACTLY,
	FITS_EXACTLY_BUT_FLIPPED
};

struct rect_wh {
	rect_wh(const rect_ltrb&);
	rect_wh(const rect_xywh&);
	rect_wh(int w = 0, int h = 0);
	int w, h, area(), perimeter();

	rect_wh_fitting fits(const rect_wh& r, const bool allow_flip) const {
		if (w == r.w && h == r.h) {
			return rect_wh_fitting::FITS_EXACTLY;
		}

		if (allow_flip) {
			if (h == r.w && w == r.h) {
				return rect_wh_fitting::FITS_EXACTLY_BUT_FLIPPED;	
			}
		}

		if (w <= r.w && h <= r.h) {
			return rect_wh_fitting::FITS_INSIDE;
		}

		if (allow_flip) {
			if (h <= r.w && w <= r.h) {
				return rect_wh_fitting::FITS_INSIDE_BUT_FLIPPED;
			}
		}

		return rect_wh_fitting::TOO_BIG;
	}
};

struct rect_ltrb {
	rect_ltrb();
	rect_ltrb(int left, int top, int right, int bottom);
	int l, t, r, b, w() const, h() const, area() const, perimeter() const;
	void w(int), h(int);
};

struct rect_xywh : public rect_wh {
	rect_xywh();
	rect_xywh(const rect_ltrb&);
	rect_xywh(int x, int y, int width, int height);
	operator rect_ltrb();

	int x, y, r() const, b() const;
	void r(int), b(int);
};

struct rect_xywhf : public rect_xywh {
	rect_xywhf(const rect_ltrb&);
	rect_xywhf(int x, int y, int width, int height);
	rect_xywhf();
	void flip();
	bool flipped;
};

rect_wh::rect_wh(const rect_ltrb& rr) : w(rr.w()), h(rr.h()) {} 
rect_wh::rect_wh(const rect_xywh& rr) : w(rr.w), h(rr.h) {} 
rect_wh::rect_wh(int w, int h) : w(w), h(h) {}

rect_ltrb::rect_ltrb() : l(0), t(0), r(0), b(0) {}
rect_ltrb::rect_ltrb(int l, int t, int r, int b) : l(l), t(t), r(r), b(b) {}

int rect_ltrb::w() const {
	return r-l;
}

int rect_ltrb::h() const {
	return b-t;
}

int rect_ltrb::area() const {
	return w()*h();
}

int rect_ltrb::perimeter() const {
	return 2*w() + 2*h();
}

void rect_ltrb::w(int ww) {
	r = l+ww;
}

void rect_ltrb::h(int hh) {
	b = t+hh;
}

rect_xywh::rect_xywh() : x(0), y(0) {}
rect_xywh::rect_xywh(const rect_ltrb& rc) : x(rc.l), y(rc.t) { b(rc.b); r(rc.r); }
rect_xywh::rect_xywh(int x, int y, int w, int h) : rect_wh(w, h), x(x), y(y) {}

rect_xywh::operator rect_ltrb() {
	rect_ltrb rr(x, y, 0, 0);
	rr.w(w); rr.h(h);
	return rr;
}

int rect_xywh::r() const {
	return x+w;
};

int rect_xywh::b() const {
	return y+h;
}

void rect_xywh::r(int right) {
	w = right-x;
}

void rect_xywh::b(int bottom) {
	h = bottom-y;
}

int rect_wh::area() {
	return w*h;
}

int rect_wh::perimeter() {
	return 2*w + 2*h; 
}


rect_xywhf::rect_xywhf(const rect_ltrb& rr) : rect_xywh(rr), flipped(false) {}
rect_xywhf::rect_xywhf(int x, int y, int width, int height) : rect_xywh(x, y, width, height), flipped(false) {}
rect_xywhf::rect_xywhf() : flipped(false) {}

void rect_xywhf::flip() { 
	flipped = !flipped;
	std::swap(w, h);
}