# rectpack2D

(This is work in progress)

Tiny rectangle packing library.
This is a refactored branch of the library that is easier to use and customize.

(The previously existing example was out of date, though, so it's been removed)

The multiple-bin functionality was removed. It is now up to you what is to be done with unsuccessful insertions.

If you pass no comparators whatsoever, the standard collection of 6 orders - by area, by perimeter, by bigger side, by width, by height, by "pathological multiplier" - will be passed by default.

You can also manually perform insertions, avoiding the need to create a vector of pointers:

```cpp
	std::vector<rect_xywhf> rects_for_packer;

	//... now fill rects_for_packer with inputs
	// and then pack it:

	auto packing_root = rectpack2D::root_node<true /* allows flip */>({ max_size, max_size });

	vec2i result_size;
	
	for (auto& rr : rects_for_packer) {
		rr.w += rect_padding_amount;
		rr.h += rect_padding_amount;

		if (const auto n = packing_root.insert(rr)) {
			rr = *n;

			rr.w -= rect_padding_amount;
			rr.h -= rect_padding_amount;
		}
		else {
			// A rectangle did not fit. Do what you like.
			break;
		}
	}
````
