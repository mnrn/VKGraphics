/**
 * @brief Common Definitions.
 */

#ifndef COMMON_H
#define COMMON_H

// ********************************************************************************
// Including files
// ********************************************************************************

// ********************************************************************************
// Definitions
// ********************************************************************************

#define UNUSED_VARIABLE(v) (static_cast<void>(v))

// ********************************************************************************
// Constant variables
// ********************************************************************************

namespace Default {
namespace Window {
static constexpr int Width = 1280;
static constexpr int Height = 720;
} // namespace Window
namespace Sampling {
static constexpr int Num = 0;
}
} // namespace Default

#endif // COMMON_H
