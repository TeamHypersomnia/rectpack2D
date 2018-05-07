#pragma once
#include <optional>
#include <vector>
#include <array>
#include <variant>

#include "insert_and_split.h"
#include "empty_space_tree.h"
#include "empty_space_allocators.h"

namespace rectpack2D {
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
	std::variant<total_area_type, rect_wh> best_packing_for_ordering_impl(
		root_node_type& root,
		F for_each_rect,
		const rect_wh starting_bin,
		const int discard_step,
		const int in_direction
	) {
		auto candidate_bin = starting_bin;

		int starting_step = 0;

		if (in_direction == 0) {
			starting_step = starting_bin.w / 2;

			candidate_bin.w /= 2;
			candidate_bin.h /= 2;
		}
		else if(in_direction == 1) {
			starting_step = starting_bin.w / 2;

			candidate_bin.w /= 2;
		}
		else {
			starting_step = starting_bin.h / 2;

			candidate_bin.h /= 2;
		}

		for (int step = starting_step; ; step = std::max(1, step / 2)) {
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

				if (in_direction == 0) {
					candidate_bin.w -= step;
					candidate_bin.h -= step;
				}
				else if (in_direction == 1) {
					candidate_bin.w -= step;
				}
				else {
					candidate_bin.h -= step;
				}

				root.reset(candidate_bin);
			}
			else {
				if (in_direction == 0) {
					candidate_bin.w += step;
					candidate_bin.h += step;

					if (candidate_bin.area() > starting_bin.area()) {
						return total_inserted_area;
					}
				}
				else if (in_direction == 1) {
					candidate_bin.w += step;

					if (candidate_bin.w > starting_bin.w) {
						return total_inserted_area;
					}
				}
				else {
					candidate_bin.h += step;

					if (candidate_bin.h > starting_bin.h) {
						return total_inserted_area;
					}
				}

			}
		}
	}

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
		const auto try_pack = [&](const int i, const rect_wh from_bin) {
			return best_packing_for_ordering_impl(root, for_each_rect, from_bin, discard_step, i);
		};

		const auto best_result = try_pack(0, starting_bin);

		if (const auto failed = std::get_if<total_area_type>(&best_result)) {
			return *failed;
		}

		auto best_bin = std::get<rect_wh>(best_result);

		for (int i = 1; i < 3; ++i) {
			const auto trial = try_pack(i, best_bin);

			if (const auto better = std::get_if<rect_wh>(&trial)) {
				best_bin = *better;
			}
		}

		return best_bin;
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
