#pragma once

// libstdc++ provides some constexpr math functions as an extension, so
// use them if we can.
#ifdef __GLIBCXX__
	#define HAVE_CONSTEXPR_STD_MATH
#endif
