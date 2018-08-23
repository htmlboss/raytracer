#pragma once

#include "Defines.h"

#include <cstdint>

namespace math_constexpr {
	// Compile-time square root using Newton-Raphson, adapted from
	// https://gist.github.com/alexshtf/eb5128b3e3e143187794
	constexpr float sqrt(const float val) {
#ifdef HAVE_CONSTEXPR_STD_MATH
		return std::sqrt(val);
#else
		auto curr{ val };
		auto prev{ 0.0f };

		while (curr != prev) {
			prev = curr;
			curr = 0.5f * (curr + val / curr);
		}

		return curr;
#endif
	}

	constexpr float floor(const float val) {
#ifdef HAVE_CONSTEXPR_STD_MATH
		return std::floor(val);
#else
		// This is wrong for anything outside the range of intmax_t
		return static_cast<std::intmax_t>(val >= 0.0f ? val : val - 1.0f);
#endif
	}

	constexpr float pow(float base, int iexp) {
#ifdef HAVE_CONSTEXPR_STD_MATH
		return std::pow(base, iexp);
#else
		while (iexp-- > 0) {
			base *= base;
		}
		return base;
#endif
	}

}