#pragma once
#include <optional>
#include <vector>
#include <array>
#include <variant>

#include "pack_structs.h"
#include "empty_spaces.h"

namespace rectpack2D {
	struct insert_result {
		int count = 0;
		std::array<space_rect, 2> space_remainders;

		static auto failed() {
			insert_result result;
			result.count = -1;
			return result;
		}

		template <class... Args>
		insert_result(Args&&... args) : space_remainders({ std::forward<Args>(args)... }) {
			count = sizeof...(Args);
		}

		bool better_than(const insert_result& b) const {
			return count < b.count;
		}

		explicit operator bool() const {
			return count != -1;
		}
	};

	inline auto insert(
		const rect_wh& im, /* Image rectangle */
		const space_rect& sp /* Space rectangle */
	) {
		const auto free_w = sp.w - im.w;
		const auto free_h = sp.h - im.h;

		if (free_w < 0 || free_h < 0) {
			return insert_result::failed();
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
			current_aabb = {};

			empty_spaces.reset();
			empty_spaces.add(rect_xywh(0, 0, r.w, r.h));
		}

		template <class F>
		std::optional<output_rect_type> insert(const rect_wh image_rectangle, F report_candidate_empty_space) {
			for (int i = empty_spaces.get_count() - 1; i >= 0; --i) {
				const auto candidate_space = empty_spaces.get(i);

				report_candidate_empty_space(candidate_space);

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
					return rectpack2D::insert(img, candidate_space);
				};

				if constexpr(!allow_flip) {
					if (const auto normal = try_to_insert(image_rectangle)) {
						return accept_result(normal, false);
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

						if (flipped.better_than(normal)) {
							return accept_result(flipped, true);
						}

						return accept_result(normal, false);
					}

					if (normal) {
						return accept_result(normal, false);
					}

					if (flipped) {
						return accept_result(flipped, true);
					}
				}
			}

			return std::nullopt;
		}

		decltype(auto) insert(const rect_wh& image_rectangle) {
			return insert(image_rectangle, [](auto&){ });
		}

		auto get_rects_aabb() const {
			return current_aabb;
		}

		const auto& get_empty_spaces() const {
			return empty_spaces;
		}
	};

	/*
		This function will do a binary search on viable bin sizes,
		starting from max_bin_side.
		
		It will do so for all provided rectangle orders.
		Only the best order will have results written to.

		The search stops when the bin was successfully inserted into,
		AND the next candidate bin size differs from the last successful one by *less* then discard_step.

		The function reports which of the rectangles did and did not fit in the end.
	*/

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

	using total_area_type = int;

	template <
		class root_node_type, 
		class F
	>
	std::variant<total_area_type, rect_wh> best_packing_for_ordering(
		root_node_type& root,
		F for_each_rect,
		const rect_wh starting_bin,
		const int discard_step
	) {
		auto candidate_bin = starting_bin;
		candidate_bin.w /= 2;
		candidate_bin.h /= 2;

		for (int step = starting_bin.w / 2; ; step = std::max(1, step / 2)) {
			root.reset(candidate_bin);

			int total_inserted_area = 0;

			const bool all_inserted = [&]() {
				return for_each_rect([&](const auto& rect) {
					if (root.insert(rect)) {
						total_inserted_area += rect.area();
						return true;
					}
					else {
						return false;
					}
				});
			}();

			if (all_inserted) {
				/* Attempt was successful. Try with a smaller bin. */

				if (step <= discard_step) {
					return candidate_bin;
				}

				candidate_bin.w -= step;
				candidate_bin.h -= step;

				root.reset(candidate_bin);
			}
			else {
				candidate_bin.w += step;
				candidate_bin.h += step;

				if (candidate_bin.w > starting_bin.w) {
					/* 
						If we are now going to attempt packing into a bin
						that is bigger than the current best, abort.
					*/

					return total_inserted_area;
				}
			}
		}
	}


	template <
		class root_node_type, 
		class OrderType,
		class F,
		class I
	>
	rect_wh find_best_packing_impl(F for_each_order, const I input) {
		const auto max_bin = rect_wh(input.max_bin_side, input.max_bin_side);

		OrderType* best_order = nullptr;

		int best_total_inserted = -1;
		auto best_bin = max_bin;

		thread_local root_node_type root = rect_wh();

		auto get_rect = [](auto& r) -> decltype(auto) {
			/* Allow both orders that are pointers and plain objects. */
			if constexpr(std::is_pointer_v<std::remove_reference_t<decltype(r)>>) {
				return (*r);
			}
			else {
				return (r);
			}
		};

		for_each_order ([&](OrderType& current_order) {
			const auto packing = best_packing_for_ordering(
				root,
				[&](auto inserter) {
					for (auto& r : current_order) {
						if (!inserter(get_rect(r).get_wh())) {
							return false;
						}
					}

					return true;
				},
				best_bin,
				input.discard_step
			);

			if (const auto total_inserted = std::get_if<total_area_type>(&packing)) {
				/*
					Track which function inserts the most area in total,
					if all orders will fail to fit into the largest allowed bin.
				*/
				if (best_order == nullptr) {
					if (*total_inserted > best_total_inserted) {
						best_order = std::addressof(current_order);
						best_total_inserted = *total_inserted;
					}
				}
			}
			else if (const auto result_bin = std::get_if<rect_wh>(&packing)) {
				/* Save the function if it performed the best. */
				if (result_bin->area() <= best_bin.area()) {
					best_order = std::addressof(current_order);
					best_bin = *result_bin;
				}
			}
		});

		{
			assert(best_order != nullptr);
			
			root.reset(best_bin);

			for (auto& rr : *best_order) {
				auto& rect = get_rect(rr);

				if (const auto ret = root.insert(rect.get_wh())) {
					rect = *ret;

					if (!input.handle_successful_insertion(rect)) {
						break;
					}
				}
				else {
					if (!input.handle_unsuccessful_insertion(rect)) {
						break;
					}
				}
			}

			return root.get_rects_aabb();
		}
	}

	template <class T>
	using output_rect_t = typename T::output_rect_type;

	template <class root_node_type, class I>
	decltype(auto) find_best_packing_dont_sort(
		std::vector<output_rect_t<root_node_type>>& subjects,
		const I& input
	) {
		return find_best_packing_impl<root_node_type, std::remove_reference_t<decltype(subjects)>>(
			[&subjects](auto callback) { callback(subjects); },
			input
		);
	}

	template <class root_node_type, class I, class Comparator, class... Comparators>
	decltype(auto) find_best_packing(
		std::vector<output_rect_t<root_node_type>>& subjects,
		const I& input,
		Comparator comparator,
		Comparators... comparators
	) {
		using rect_type = output_rect_t<root_node_type>;
		using order_type = std::vector<rect_type*>;

		constexpr auto count_orders = 1 + sizeof...(Comparators);
		thread_local std::array<order_type, count_orders> orders;

		{
			/* order[0] will always exist since this overload requires at least one comparator */
			auto& initial_pointers = orders[0];
			initial_pointers.clear();

			for (auto& s : subjects) {
				initial_pointers.emplace_back(std::addressof(s));
			}

			for (std::size_t i = 1; i < count_orders; ++i) {
				orders[i] = initial_pointers;
			}
		}

		std::size_t f = 0;

		auto make_order = [&f](auto& predicate) {
			std::sort(orders[f].begin(), orders[f].end(), predicate);
			++f;
		};

		make_order(comparator);
		(make_order(comparators), ...);

		return find_best_packing_impl<root_node_type, order_type>(
			[](auto callback){ for (auto& o : orders) { callback(o); } },
			input
		);
	}


	template <class root_node_type, class I>
	decltype(auto) find_best_packing(
		std::vector<output_rect_t<root_node_type>>& subjects,
		const I& input
	) {
		using rect_type = output_rect_t<root_node_type>;

		return find_best_packing<root_node_type>(
			subjects,
			input,
			[](const rect_type* const a, const rect_type* const b) {
				return a->area() > b->area();
			},
			[](const rect_type* const a, const rect_type* const b) {
				return a->perimeter() > b->perimeter();
			},
			[](const rect_type* const a, const rect_type* const b) {
				return std::max(a->w, a->h) > std::max(b->w, b->h);
			},
			[](const rect_type* const a, const rect_type* const b) {
				return a->w > b->w;
			},
			[](const rect_type* const a, const rect_type* const b) {
				return a->h > b->h;
			},
			[](const rect_type* const a, const rect_type* const b) {
				return a->get_wh().pathological_mult() > b->get_wh().pathological_mult();
			}
		);
	}
}
