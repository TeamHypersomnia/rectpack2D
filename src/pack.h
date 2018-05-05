#pragma once
#include <optional>
#include <vector>
#include <array>

#include "pack_structs.h"

namespace rectpack {
	class node;
	using node_array_type = std::array<node, 10000>;

	class node {
		static int nodes_count;
		static node_array_type all_nodes;

		class child_node {
			int ptr = -1;
		public:

			bool is_allocated() const {
				return ptr != -1;
			}

			auto& get() {
				return all_nodes[ptr];
			}

			const auto& get() const {
				return all_nodes[ptr];
			}

			void set(const rect_ltrb& r) {
				if (ptr == -1) { 
					ptr = nodes_count++;
				}

				all_nodes[ptr] = r;
			}
		};

		friend class child_node;
		friend class root_node;

		enum class leaf_fill {
			TOO_BIG,
			EXACT,
			GROW
		};

		rect_ltrb rc;
		child_node child[2];
		bool leaf_filled = false;

		template <class R>
		auto get_leaf_filling(
			const R& image_rectangle,
			const bool allow_flip,
			bool& out_flipping_necessary
		) const {
			switch (image_rectangle.get_fitting(rect_xywh(rc), allow_flip)) {
				case rect_wh_fitting::TOO_BIG: 
					return leaf_fill::TOO_BIG;

				case rect_wh_fitting::FITS_INSIDE:
					out_flipping_necessary = false; 
					return leaf_fill::GROW;

				case rect_wh_fitting::FITS_INSIDE_BUT_FLIPPED: 	
					out_flipping_necessary = true; 
					return leaf_fill::GROW;

				case rect_wh_fitting::FITS_EXACTLY:
					out_flipping_necessary = false; 
					return leaf_fill::EXACT;

				case rect_wh_fitting::FITS_EXACTLY_BUT_FLIPPED: 
					out_flipping_necessary = true;  
					return leaf_fill::EXACT;
			}
		}

		template <class R>
		void grow_branch_for(const R& image_rectangle, const bool flipping_necessary) {
			const auto iw = flipping_necessary ? image_rectangle.h : image_rectangle.w;
			const auto ih = flipping_necessary ? image_rectangle.w : image_rectangle.h;

			if (rc.w() - iw > rc.h() - ih) {
				child[0].set({ rc.l, rc.t, rc.l + iw, rc.b });
				child[1].set({ rc.l + iw, rc.t, rc.r, rc.b });
			}
			else {
				child[0].set({ rc.l, rc.t, rc.r, rc.t + ih });
				child[1].set({ rc.l, rc.t + ih, rc.r, rc.b });
			}
		}

		template <class R>
		const node* leaf_insert(const R& image_rectangle, const bool allow_flip) {
			bool flipping_necessary = false;

			auto get_filling_for = [&image_rectangle, allow_flip, &flipping_necessary](const node& n) {
				return n.get_leaf_filling(
					image_rectangle, 
					allow_flip, 
					flipping_necessary
				);
			};

			const auto filling = get_filling_for(*this);

			if (filling == leaf_fill::EXACT) {
				leaf_filled = true; 
				return this;
			}
			else if (filling == leaf_fill::GROW) {
				grow_branch_for(image_rectangle, flipping_necessary);

				auto& new_leaf = child[0].get();

				const auto second_filling = get_filling_for(new_leaf);

				if (second_filling == leaf_fill::GROW) {
					new_leaf.grow_branch_for(image_rectangle, flipping_necessary);

					/* Left leaf of the new child must fit exactly by this point. */

					auto& target_leaf = new_leaf.child[0].get();
					target_leaf.leaf_filled = true;
					return &target_leaf;
				}
				else if (second_filling == leaf_fill::EXACT) {
					new_leaf.leaf_filled = true;
					return &new_leaf;
				}
				else {
					/* 
						Should never happen. 
						Since the first fitting query has determined that the image would fit,
						the subsequently grown branch could not have been too small.
					*/
				}
			}

			/* Control may only reach here when the image was too big. */
			return nullptr;
		}

		bool is_empty_leaf() const {
			return !child[0].is_allocated() && !child[1].is_allocated() && !leaf_filled;
		}

		node(rect_ltrb rc) : rc(rc) {}

	public:
		node() = default;

		template <class T>
		void readback(
			rect_xywhf& into,
			T& tracked_dimensions
		) const {
			into.x = rc.l;
			into.y = rc.t;
			into.flipped = (rc.w() == into.h && rc.h() == into.w);

			if (into.flipped) {
				std::swap(into.w, into.h);
			}

			tracked_dimensions.x = std::max(tracked_dimensions.x, rc.r);
			tracked_dimensions.y = std::max(tracked_dimensions.y, rc.b); 
		}
	};

	class root_node {
		node first;

	public:
		root_node(const rect_wh& r) : first(node({0, 0, r.w, r.h})) {
			rectpack::node::nodes_count = 0;
		};

		template <class R>
		const node* insert(const R& image_rectangle, const bool allow_flip) {
			if (first.is_empty_leaf()) {
				/* Will happen only for the first time. */
				return first.leaf_insert(image_rectangle, allow_flip);
			}

			/* Recently allocated nodes are more likely to be empty leaves. */

			for (int i = node::nodes_count - 1; i >= 0; --i) {
				auto& candidate = node::all_nodes[i];

				if (candidate.is_empty_leaf()) {
					if (const auto result = candidate.leaf_insert(image_rectangle, allow_flip)) {
						return result;
					}
				}
			}

			return nullptr;
		}

		auto current_size() const {
			return first.rc;
		}
	};

	template <class F, class G, class... Comparators>
	rect_wh pack_rectangles(
		const std::vector<rect_xywhf*>& input, 
		const int max_bin_side, 
		const bool allow_flip, 
		F push_successful,
		G push_unsuccessful,
		const int discard_step,
		Comparators... comparators
	) {
		constexpr auto funcs = sizeof...(Comparators);
		const auto n = input.size();

		thread_local std::vector<rect_xywhf*> order[funcs];

		bool (*cmpf[funcs])(rect_xywhf*, rect_xywhf*) = {
			comparators...
		};

		for (std::size_t f = 0; f < funcs; ++f) { 
			order[f] = input;
			std::sort(order[f].begin(), order[f].end(), cmpf[f]);
		}

		rect_wh min_bin = rect_wh(max_bin_side, max_bin_side);

		std::optional<int> min_func;

		int	best_func = 0;
		int best_area = 0;
		int current_area = 0;

		bool fail = false;

		for (unsigned f = 0; f < funcs; ++f) {
			const auto& v = order[f];

			int step = min_bin.w / 2;
			auto root = root_node(min_bin);

			while (true) {
				if (root.current_size().w() > min_bin.w) {
					/* 
						If we are now going to attempt packing into a bin
						that is bigger than the current minimum, abort.
					*/

					if (min_func) {
						break;
					}

					current_area = 0;

					root = root_node(min_bin);

					for (std::size_t i = 0; i < n; ++i) {
						if (root.insert(*v[i], allow_flip)) {
							current_area += v[i]->area();
						}
					}

					fail = true;
					break;
				}

				const bool all_inserted = [&]() {
					for (std::size_t i = 0; i < n; ++i) {
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
					root = root_node({ root.current_size().w() - step, root.current_size().h() - step });
				}
				else {
					/* Attempt ended in failure. Try with a bigger bin. */
					root = root_node({ root.current_size().w() + step, root.current_size().h() + step });
				}

				step /= 2;

				if (!step) {
					step = 1;
				}
			}

			if (!fail && (min_bin.area() >= root.current_size().area())) {
				min_bin = rect_wh(root.current_size());
				min_func = f;
			}
			else if (fail && (current_area > best_area)) {
				best_area = current_area;
				best_func = f;
			}

			fail = false;
		}

		struct {
			int x = 0;
			int y = 0;
		} clip;

		{
			auto& v = order[min_func ? *min_func : best_func];

			auto root = root_node(min_bin);

			for (std::size_t i = 0; i < n; ++i) {
				if (const auto ret = root.insert(*v[i],allow_flip)) {
					ret->readback(*v[i], clip);

					if (!push_successful(v[i])) {
						break;
					}
				}
				else {
					if (!push_unsuccessful(v[i])) {
						break;
					}

					v[i]->flipped = false;
				}
			}
		}

		return rect_wh(clip.x, clip.y);
	}

	template <class F, class G>
	rect_wh pack_rectangles(
		const std::vector<rect_xywhf*>& input, 
		const int max_bin_side, 
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

		return pack_rectangles(
			input,

			max_bin_side,
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
}

rectpack::node_array_type rectpack::node::all_nodes;
int rectpack::node::nodes_count = 0;
