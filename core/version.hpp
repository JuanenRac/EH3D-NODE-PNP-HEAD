// EH3D-NODE-PNP-HEAD - core/version.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The version of the firmware. The firmware build and the host tests define PNP_HEAD_VERSION from eh3d.project.json, the one place that says it;
// a test of the repository fails when the fallback below is not that version.
#pragma once

#ifndef PNP_HEAD_VERSION
#define PNP_HEAD_VERSION "0.0.3"
#endif

namespace pnp_head {
constexpr const char* kVersion = PNP_HEAD_VERSION;
}
