#pragma once
#include <optional>
#include <vector>

#include "3rdparty/rectpack2D/src/pack_structs.h"

struct node {
	struct pnode {
		node* pn = nullptr;
		bool fill = false;

		void set(const int l, const int t, const int r, const int b) {
			if(!pn) pn = new node(rect_ltrb(l, t, r, b));
			else {
				(*pn).rc = rect_ltrb(l, t, r, b);
				(*pn).id = false;
			}
			fill = true;
		}
	};

	pnode c[2];
	rect_ltrb rc;
	bool id = false;
	node(rect_ltrb rc = rect_ltrb()) : rc(rc) {}

	void reset(const rect_wh& r) {
		id = false;
		rc = rect_ltrb(0, 0, r.w, r.h);
		delcheck();
	}

	node* insert(rect_xywhf& img, const bool allow_flip) {
		if(c[0].pn && c[0].fill) {
			if(auto newn = c[0].pn->insert(img,allow_flip)) return newn;
			return    c[1].pn->insert(img,allow_flip);
		}

		if(id) return 0;
		int f = img.fits(rect_xywh(rc),allow_flip);

		switch(f) {
			case 0: return 0;
			case 1: img.flipped = false; break;
			case 2: img.flipped = true; break;
			case 3: id = true; img.flipped = false; return this;
			case 4: id = true; img.flipped = true;  return this;
		}

		int iw = (img.flipped ? img.h : img.w), ih = (img.flipped ? img.w : img.h);

		if(rc.w() - iw > rc.h() - ih) {
			c[0].set(rc.l, rc.t, rc.l+iw, rc.b);
			c[1].set(rc.l+iw, rc.t, rc.r, rc.b);
		}
		else {
			c[0].set(rc.l, rc.t, rc.r, rc.t + ih);
			c[1].set(rc.l, rc.t + ih, rc.r, rc.b);
		}

		return c[0].pn->insert(img,allow_flip);
	}

	void delcheck() {
		if(c[0].pn) { c[0].fill = false; c[0].pn->delcheck(); }
		if(c[1].pn) { c[1].fill = false; c[1].pn->delcheck(); }
	}

	~node() {
		if(c[0].pn) delete c[0].pn;
		if(c[1].pn) delete c[1].pn;
	}
};

template <class F, class G, class... Comparators>
rect_wh pack_rectangles(
	const std::vector<rect_xywhf*>& input, 
	const int max_s, 
	const bool allow_flip, 
	F push_successful,
	G push_unsuccessful,
	const int discard_step,
	Comparators... comparators
) {
	node root;
	const int n = input.size();

	constexpr auto funcs = sizeof...(Comparators);

	bool (*cmpf[funcs])(rect_xywhf*, rect_xywhf*) = {
		comparators...
	};

	thread_local std::vector<rect_xywhf*> order[funcs];

	for (unsigned f = 0; f < funcs; ++f) { 
		order[f] = input;
		std::sort(order[f].begin(), order[f].end(), cmpf[f]);
	}

	rect_wh min_bin = rect_wh(max_s, max_s);

	std::optional<int> min_func;

	int	best_func = 0;
	int best_area = 0;

	int current_area = 0;

	bool fail = false;

	for (unsigned f = 0; f < funcs; ++f) {
		const auto& v = order[f];

		int step = min_bin.w / 2;
		root.reset(min_bin);

		while (true) {
			if (root.rc.w() > min_bin.w) {
				/* 
					If we are now going to attempt packing into a bin
					that is bigger than the current minimum, abort.
				*/

				if (min_func) {
					break;
				}

				current_area = 0;

				root.reset(min_bin);

				for (int i = 0; i < n; ++i) {
					if (root.insert(*v[i],allow_flip)) {
						current_area += v[i]->area();
					}
				}

				fail = true;
				break;
			}

			const bool all_inserted = [&]() {
				for (int i = 0; i < n; ++i) {
					if (!root.insert(*v[i], allow_flip)) {
						return false;
					}
				}

				return true;
			}();

			if (all_inserted) {
				if (step <= discard_step) {
					break;
				}

				/* Attempt was successful. Try with a smaller bin. */
				root.reset({ root.rc.w() - step, root.rc.h() - step });
			}
			else {
				/* Attempt ended in failure. Try with a bigger bin. */
				root.reset({ root.rc.w() + step, root.rc.h() + step });
			}

			step /= 2;

			if (!step) {
				step = 1;
			}
		}

		if (!fail && (min_bin.area() >= root.rc.area())) {
			min_bin = rect_wh(root.rc);
			min_func = f;
		}
		else if (fail && (current_area > best_area)) {
			best_area = current_area;
			best_func = f;
		}

		fail = false;
	}

	int clip_x = 0;
	int clip_y = 0;

	{
		auto& v = order[min_func ? *min_func : best_func];

		root.reset(min_bin);

		for (int i = 0; i < n; ++i) {
			if(auto ret = root.insert(*v[i],allow_flip)) {
				v[i]->x = ret->rc.l;
				v[i]->y = ret->rc.t;

				if(v[i]->flipped) {
					v[i]->flipped = false;
					v[i]->flip();
				}

				clip_x = std::max(clip_x, ret->rc.r);
				clip_y = std::max(clip_y, ret->rc.b); 

				push_successful(v[i]);
			}
			else {
				push_unsuccessful(v[i]);

				v[i]->flipped = false;
			}
		}
	}

	return rect_wh(clip_x, clip_y);
}

template <class F, class G>
rect_wh pack_rectangles(
	const std::vector<rect_xywhf*>& input, 
	const int max_s, 
	const bool allow_flip, 
	F push_successful,
	G push_unsuccessful,
	const int discard_step
) {
	auto area = [](rect_xywhf* a, rect_xywhf* b) {
		return a->area() > b->area();
	};

	auto perimeter = [](rect_xywhf* a, rect_xywhf* b) {
		return a->perimeter() > b->perimeter();
	};

	auto max_side = [](rect_xywhf* a, rect_xywhf* b) {
		return std::max(a->w, a->h) > std::max(b->w, b->h);
	};

	auto max_width = [](rect_xywhf* a, rect_xywhf* b) {
		return a->w > b->w;
	};

	auto max_height = [](rect_xywhf* a, rect_xywhf* b) {
		return a->h > b->h;
	};

	return _rect2D(
		input,

		max_s,
		allow_flip,
		push_successful,
		push_unsuccessful,
		discard_step,

		area,
		perimeter,
		max_side,
		max_width,
		max_height
	);
}
