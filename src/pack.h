#pragma once
#include <optional>
#include <vector>
#include <array>

#include "pack_structs.h"
#include "empty_spaces.h"

namespace rectpack {
	struct insert_result {
		int count = 0;
		std::array<space_rect, 2> space_remainders;

		template <class... Args>
		insert_result(Args&&... args) : space_remainders({ std::forward<Args>(args)... }) {
			count = sizeof...(Args);
		}

		bool better_than(const insert_result& b) const {
			return count < b.count;
		}
	};

	inline std::optional<insert_result> insert(
		const rect_wh im, /* Image rectangle */
		const space_rect sp /* Space rectangle */
	) {
		const auto free_w = sp.w - im.w;
		const auto free_h = sp.h - im.h;

		if (free_w < 0 || free_h < 0) {
			return std::nullopt;
		}

		if (free_w == 0 && free_h == 0) {
			return insert_result();
		}

		if (free_w > 0 && free_h == 0) {
			auto r = sp;
			r.x += im.w;
			r.w -= im.w;
			return insert_result(r);
		}

		if (free_w == 0 && free_h > 0) {
			auto r = sp;
			r.y += im.h;
			r.h -= im.h;
			return insert_result(r);
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

			return insert_result(bigger_split, lesser_split);
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

		return insert_result(bigger_split, lesser_split);
	}

	template <bool allow_flip, class empty_spaces_provider = default_empty_spaces>
	class root_node {
		empty_spaces_provider empty_spaces;

	public:
		using output_rect_type = std::conditional_t<allow_flip, rect_xywhf, rect_xywh>;

	private:
		rect_wh initial_size;
		rect_wh current_aabb;

		void expand_aabb_with(const output_rect_type& result) {
			current_aabb.w = std::max(current_aabb.w, result.x + result.w);
			current_aabb.h = std::max(current_aabb.h, result.y + result.h); 
		}

	public:
		root_node(const rect_wh& r) {
			reset(r);
		}

		void reset(const rect_wh& r) {
			initial_size = r;
			current_aabb = {};

			empty_spaces.reset();
			empty_spaces.add(rect_xywh(0, 0, r.w, r.h));
		}

		std::optional<output_rect_type> insert(const rect_wh image_rectangle) {
			for (int i = empty_spaces.get_count() - 1; i >= 0; --i) {
				const auto candidate_space = empty_spaces.get(i);

				auto accept_result = [this, i, image_rectangle, candidate_space](
					const insert_result& inserted,
					const bool flipping_necessary
				) -> std::optional<output_rect_type> {
					empty_spaces.remove(i);

					for (int s = 0; s < inserted.count; ++s) {
						if (!empty_spaces.add(inserted.space_remainders[s])) {
							return std::nullopt;
						}
					}

					if constexpr(allow_flip) {
						const auto result = output_rect_type(
							candidate_space.x,
							candidate_space.y,
							image_rectangle.w,
							image_rectangle.h,
							flipping_necessary
						);

						expand_aabb_with(result);
						return result;
					}
					else {
						(void)flipping_necessary;

						const auto result = output_rect_type(
							candidate_space.x,
							candidate_space.y,
							image_rectangle.w,
							image_rectangle.h
						);

						expand_aabb_with(result);
						return result;
					}
				};

				auto try_to_insert = [&](const rect_wh& img) {
					return rectpack::insert(img, candidate_space);
				};

				if constexpr(!allow_flip) {
					if (const auto normal = try_to_insert(image_rectangle)) {
						return accept_result(*normal, false);
					}
				}
				else {
					const auto normal = try_to_insert(image_rectangle);
					const auto flipped = try_to_insert(rect_wh(image_rectangle).flip());

					/* 
						If both were successful, 
						prefer the one that generated less remainder spaces.
					*/

					if (normal && flipped) {
						/* 
							To prefer normal placements instead of flipped ones,
							better_than will return true only if the flipped one generated
						   	*strictly* less space remainders.
						*/

						if (flipped->better_than(*normal)) {
							return accept_result(*flipped, true);
						}

						return accept_result(*normal, false);
					}

					if (normal) {
						return accept_result(*normal, false);
					}

					if (flipped) {
						return accept_result(*flipped, true);
					}
				}
			}

			return std::nullopt;
		}

		auto get_size() const {
			return initial_size;
		}

		auto get_rects_aabb() const {
			return current_aabb;
		}
	};

	/*
		This function will do a binary search on viable bin sizes,
		starting from max_bin_side.
		
		The search stops when the bin was successfully inserted into,
		AND the next candidate bin size differs from the last successful one by *less* then discard_step.
	*/

	template <class root_node_type, class F, class G, class... Comparators>
	rect_wh find_best_packing(
		const std::vector<typename root_node_type::output_rect_type*>& input, 
		const int max_bin_side, 
		F push_successful,
		G push_unsuccessful,
		const int discard_step,
		Comparators... comparators
	) {
		using rect_type = typename root_node_type::output_rect_type;

		constexpr auto count_funcs = sizeof...(Comparators);
		thread_local std::array<std::vector<rect_type*>, count_funcs> order;

		{
			std::size_t f = 0;

			auto make_order = [&f, &input](auto& comparator) {
				order[f] = input;
				std::sort(order[f].begin(), order[f].end(), comparator);
				++f;
			};

			(make_order(comparators), ...);
		}

		const auto n = input.size();

		rect_wh min_bin = rect_wh(max_bin_side, max_bin_side);

		std::optional<int> min_func;

		int	best_func = 0;
		int best_area = 0;
		int current_area = 0;

		bool fail = false;

		thread_local root_node_type root = rect_wh();
		root.reset(min_bin);

		for (unsigned f = 0; f < order.size(); ++f) {
			const auto& v = order[f];

			int step = min_bin.w / 2;
			root.reset(min_bin);

			while (true) {
				if (root.get_size().w > min_bin.w) {
					/* 
						If we are now going to attempt packing into a bin
						that is bigger than the current minimum, abort.
					*/

					if (min_func) {
						break;
					}

					current_area = 0;

					root.reset(min_bin);

					for (std::size_t i = 0; i < n; ++i) {
						if (root.insert(v[i]->get_wh())) {
							current_area += v[i]->area();
						}
					}

					fail = true;
					break;
				}

				const bool all_inserted = [&]() {
					for (std::size_t i = 0; i < n; ++i) {
						if (!root.insert(v[i]->get_wh())) {
							return false;
						}
					}

					return true;
				}();

				if (all_inserted) {
					if (step <= discard_step) {
						break;
					}

					auto new_size = root.get_size();
					new_size.w -= step;
					new_size.h -= step;

					/* Attempt was successful. Try with a smaller bin. */
					root.reset(new_size);
				}
				else {
					auto new_size = root.get_size();
					new_size.w += step;
					new_size.h += step;

					/* Attempt ended in failure. Try with a bigger bin. */
					root.reset(new_size);
				}

				step /= 2;

				if (!step) {
					step = 1;
				}
			}

			if (!fail && (min_bin.area() >= root.get_size().area())) {
				min_bin = root.get_size();
				min_func = f;
			}
			else if (fail && (current_area > best_area)) {
				best_area = current_area;
				best_func = f;
			}

			fail = false;
		}

		{
			const auto& v = order[min_func ? *min_func : best_func];

			root.reset(min_bin);

			for (std::size_t i = 0; i < n; ++i) {
				if (const auto ret = root.insert(v[i]->get_wh())) {
					*v[i] = *ret;

					if (!push_successful(v[i])) {
						break;
					}
				}
				else {
					if (!push_unsuccessful(v[i])) {
						break;
					}
				}
			}

			return root.get_rects_aabb();
		}
	}

	template <class root_node_type, class... Args>
	rect_wh find_best_packing_default(Args&&... args) {
		using rect_type = typename root_node_type::output_rect_type;

		auto area = [](const rect_type* const a, const rect_type* const b) {
			return a->area() > b->area();
		};

		auto perimeter = [](const rect_type* const a, const rect_type* const b) {
			return a->perimeter() > b->perimeter();
		};

		auto max_side = [](const rect_type* const a, const rect_type* const b) {
			return std::max(a->w, a->h) > std::max(b->w, b->h);
		};

		auto max_width = [](const rect_type* const a, const rect_type* const b) {
			return a->w > b->w;
		};

		auto max_height = [](const rect_type* const a, const rect_type* const b) {
			return a->h > b->h;
		};

		return find_best_packing<root_node_type>(
			std::forward<Args>(args)...,
			area,
			perimeter,
			max_side,
			max_width,
			max_height
		);
	}
}
