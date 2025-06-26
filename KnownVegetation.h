#pragma once
#include <set>
#include <cstdint>

extern std::set<uint32_t> knownVegetationHashes;
extern std::set<uint32_t> knownLeafTextures;
extern uint32_t g_lastTextureHash;

void LoadKnownLeafTextures(); // 