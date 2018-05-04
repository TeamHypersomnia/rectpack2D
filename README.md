# rectpack2D

Tiny rectangle packing library.
This a refactored branch of the library that is easier to use and customize.

The example was out of date, though, so it's been removed.

The multiple-bin functionality was removed. It is now up to you what is to be done with unsuccessful insertions:

```cpp
	template <class F, class G, class... Comparators>
	rect_wh pack_rectangles(
		const std::vector<rect_xywhf*>& input, 
		const int max_bin_side, 
		const bool allow_flip, 
		F push_successful,
		G push_unsuccessful,
		const int discard_step,
		Comparators... comparators
	)
````

If you pass no comparators whatsoever, the standard collection of 5 orders - by area, by perimeter, by bigger side, by width, by height - will be passed by default.

You can also manually perform insertions, avoiding the need to create a vector of pointers:

```cpp
	auto packing_root = rectpack::node::make_root({ max_size, max_size });

	vec2i result_size;
	
	for (auto& rr : rects_for_packing_algorithm) {
		rr.w += rect_padding_amount;
		rr.h += rect_padding_amount;

		if (const auto n = packing_root.insert(rr, true)) {
			n->readback(rr, result_size);

			rr.w -= rect_padding_amount;
			rr.h -= rect_padding_amount;
		}
		else {
			// A rectangle did not fit.
			break;
		}
	}
````
