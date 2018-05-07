#pragma once
#include <array>
#include "rect_structs.h"

namespace rectpack2D {
	struct created_splits {
		int count = 0;
		std::array<space_rect, 2> spaces;

		static auto failed() {
			created_splits result;
			result.count = -1;
			return result;
		}

		static auto none() {
			return created_splits();
		}

		template <class... Args>
		created_splits(Args&&... args) : spaces({ std::forward<Args>(args)... }) {
			count = sizeof...(Args);
		}

		bool better_than(const created_splits& b) const {
			return count < b.count;
		}

		explicit operator bool() const {
			return count != -1;
		}
	};

	inline created_splits insert_and_split(
		const rect_wh& im, /* Image rectangle */
		const space_rect& sp /* Space rectangle */
	) {
		const auto free_w = sp.w - im.w;
		const auto free_h = sp.h - im.h;

		if (free_w < 0 || free_h < 0) {
			return created_splits::failed();
		}

		if (free_w == 0 && free_h == 0) {
			return created_splits::none();
		}

		if (free_w > 0 && free_h == 0) {
			auto r = sp;
			r.x += im.w;
			r.w -= im.w;
			return created_splits(r);
		}

		if (free_w == 0 && free_h > 0) {
			auto r = sp;
			r.y += im.h;
			r.h -= im.h;
			return created_splits(r);
		}

		/* 
			At this point both free_w and free_h must be positive.
			Decide which way to split.
		*/

		if (free_w > free_h) {
			const auto bigger_split = space_rect(
				sp.x + im.w,
				sp.y,
			   	free_w,
			   	sp.h
			);

			const auto lesser_split = space_rect(
				sp.x,
				sp.y + im.h,
				im.w,
				free_h
			);

			return created_splits(bigger_split, lesser_split);
		}

		const auto bigger_split = space_rect(
			sp.x,
			sp.y + im.h,
			sp.w,
			free_h
		);

		const auto lesser_split = space_rect(
			sp.x + im.w,
			sp.y,
			free_w,
			im.h
		);

		return created_splits(bigger_split, lesser_split);
	}
}
