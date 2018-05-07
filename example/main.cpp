#include <iostream>
#include "../src/finders_interface.h"

using namespace rectpack2D;

int main() {
	constexpr bool allow_flip = true;
	using root_type = rectpack2D::root_node<allow_flip>;

	/* rect_xywh or rect_xywhf, depending on the value of allow_flip */
	using rect_type = output_rect_t<root_type>;

	using rect_ptr = rect_type*;

	auto report_successful = [](rect_type& rect) {
		/* Continue */
		return true;
	};

	auto report_unsuccessful = [](rect_type& rect) {
		/* Abort */
		return false;
	};

	const auto max_side = 1000;
	const auto discard_step = 1;

	std::vector<rect_type> rectangles;

	/* Add some arbitrary rectangles */

	rectangles.emplace_back(rect_xywh(0, 0, 20, 40));
	rectangles.emplace_back(rect_xywh(0, 0, 120, 40));
	rectangles.emplace_back(rect_xywh(0, 0, 85, 59));
	rectangles.emplace_back(rect_xywh(0, 0, 199, 380));
	rectangles.emplace_back(rect_xywh(0, 0, 85, 875));
	
	{
		/* Example 1: Find best packing with default orders. */
		const auto result_size = find_best_packing<root_type>(
			rectangles,
			make_finder_input(
				max_side,
				discard_step,
				report_successful,
				report_unsuccessful
			)
		);

		std::cout << result_size.w << " " << result_size.h << std::endl;
	}

	{
		/* Example 2: Find best packing with custom orders. */

		auto my_custom_order_1 = [](const rect_ptr a, const rect_ptr b) {
			return a->get_wh().pathological_mult() > b->get_wh().pathological_mult();
		};

		auto my_custom_order_2 = [](const rect_ptr a, const rect_ptr b) {
			return a->get_wh().pathological_mult() < b->get_wh().pathological_mult();
		};

		const auto result_size = find_best_packing<root_type>(
			rectangles,
			make_finder_input(
				max_side,
				discard_step,
				report_successful,
				report_unsuccessful
			),

			my_custom_order_1,
			my_custom_order_2
		);

		std::cout << result_size.w << " " << result_size.h << std::endl;
	}

	{
		/* Example 3: Find best packing exactly in the order of provided input. */

		const auto result_size = find_best_packing_dont_sort<root_type>(
			rectangles,
			make_finder_input(
				max_side,
				discard_step,
				report_successful,
				report_unsuccessful
			)
		);

		std::cout << result_size.w << " " << result_size.h << std::endl;
	}

	{
		/* Example 4: Manually perform insertions. This way you can try your own best-bin finding logic. */

		auto packing_root = root_type({ 1000, 1000 });

		for (auto& r : rectangles) {
			if (const auto n = packing_root.insert(std::as_const(r).get_wh())) {
				r = *n;
			}
			else {
				std::cout << "Failed to insert a rectangle." << std::endl;
				break;
			}
		}

		const auto result_size = packing_root.get_rects_aabb();

		std::cout << result_size.w << " " << result_size.h << std::endl;
	}

	std::cout << "TEST" << std::endl;
	return 0;
}
