#pragma once

#include <atomic>
#include <cstdint>
#include <string>

// The mod's settings: read from SKSE\Plugins\QuickLootContainerMerge.ini, written back line by line (never with
// WritePrivateProfileString, which reports success and writes nothing under MO2's virtual folder). The page, DevBench and the
// loot window run on different threads, so every value is atomic. Compiled defaults match the shipped INI (rule 16).
namespace settings
{
	struct Defaults
	{
		static constexpr std::uint32_t logLevel = 2;
		static constexpr bool enabled = true;
		static constexpr float radius = 500.0f;
		static constexpr bool lineOfSight = false;
	};

	inline constexpr float kRadiusMin = 100.0f;
	inline constexpr float kRadiusMax = 3000.0f;

	inline std::atomic<std::uint32_t> logLevel{ Defaults::logLevel };
	inline std::atomic<bool> enabled{ Defaults::enabled };          // bEnabled:General - the whole mod
	inline std::atomic<float> radius{ Defaults::radius };           // fRadius:General - game units around what the crosshair is on
	inline std::atomic<bool> lineOfSight{ Defaults::lineOfSight };  // bLineOfSight:General - only sources the player can see

	void Load();             // at plugin load; a missing INI or line keeps the default
	bool Save();             // every value, keeping the file's comments; false when the file cannot be written
	void RestoreDefaults();  // in memory, as the page's button does; the caller saves
	const std::string& IniPath();
	void ApplyLogLevel();
}
