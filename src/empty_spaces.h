#pragma once
#include <array>
#include <vector>
#include <type_traits>

namespace rectpack {
	using space_rect = rect_xywh;

	class default_empty_spaces {
		std::vector<space_rect> spaces;

	public:
		void remove(const int i) {
			spaces[i] = spaces.back();
			spaces.pop_back();
		}

		bool add(const space_rect r) {
			spaces.emplace_back(r);
			return true;
		}

		auto count() const {
			return spaces.size();
		}

		void reset() {
			spaces.clear();
		}

		const auto& get(const int i) {
			return spaces[i];
		}
	};

	template <int MAX_SPACES>
	class static_empty_spaces {
		std::array<space_rect, MAX_SPACES> spaces;
		int num = 0;

	public:
		void remove(const int i) {
			spaces[i] = spaces[num - 1];
			--num;
		}

		bool add(const space_rect r) {
			if (num < static_cast<int>(spaces.size())) {
				spaces[num] = r;
				++num;

				return true;
			}

			return false;
		}
		
		auto count() const {
			return num;
		}

		void reset() {
			num = 0;
		}

		const auto& get(const int i) {
			return spaces[i];
		}
	};
}
