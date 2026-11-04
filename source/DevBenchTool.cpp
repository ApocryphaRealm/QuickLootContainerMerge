#include "PCH.h"

#include "DevBenchTool.h"

#include "DevBench/DevBenchAPI.h"
#include "Merge.h"
#include "Settings.h"
#include "utils/Logger.h"

#include <algorithm>
#include <format>
#include <string>
#include <string_view>

// DevBench tool "qcm.control" (rules 31 and 64): the merge's live state, and QuickLoot IE's own actions and forced container
// through its public API, so a run can drive the window with no key pressed. A real test still presses the real button.
namespace DevBenchTool
{
	namespace
	{
		// The value of a top-level JSON member, as text: a quoted string or a bare word. Empty when absent.
		std::string Get(std::string_view a_json, const char* a_name)
		{
			const std::string key = std::format("\"{}\"", a_name);
			auto pos = a_json.find(key);
			if (pos == std::string_view::npos) { return {}; }
			pos = a_json.find(':', pos + key.size());
			if (pos == std::string_view::npos) { return {}; }
			++pos;
			while (pos < a_json.size() && (a_json[pos] == ' ' || a_json[pos] == '\t')) { ++pos; }
			if (pos >= a_json.size()) { return {}; }
			if (a_json[pos] == '"')
			{
				const auto close = a_json.find('"', pos + 1);
				return close == std::string_view::npos ? std::string{} : std::string(a_json.substr(pos + 1, close - pos - 1));
			}
			std::string out;
			while (pos < a_json.size() && a_json[pos] != ',' && a_json[pos] != '}' && a_json[pos] != ' ') { out += a_json[pos++]; }
			return out;
		}

		bool SetByName(const std::string& a_name, const std::string& a_value)
		{
			try
			{
				if (a_name == "bEnabled") { settings::enabled = a_value == "1"; }
				else if (a_name == "fInteriorFeet") { settings::interiorFeet = std::clamp(std::stof(a_value), settings::kFeetMin, settings::kFeetMax); }
				else if (a_name == "fExteriorFeet") { settings::exteriorFeet = std::clamp(std::stof(a_value), settings::kFeetMin, settings::kFeetMax); }
				else if (a_name == "bLineOfSight") { settings::lineOfSight = a_value == "1"; }
				else { return false; }
			}
			catch (...)
			{
				return false;
			}
			return true;
		}

		void ControlTool(void*, const char* a_args, void* a_sink, DevBenchAPI::WriteFn a_write)
		{
			const std::string_view args = a_args ? a_args : "";
			const std::string op = Get(args, "op");
			std::string extra = "true";
			if (op == "action")
			{
				if (!merge::Action(Get(args, "name"))) { extra = "false,\"error\":\"name is take, takeall, use, up, down or transfer, and QuickLoot IE must be connected\""; }
			}
			else if (op == "focus")
			{
				std::uint32_t id = 0;
				try { id = static_cast<std::uint32_t>(std::stoul(Get(args, "ref"), nullptr, 16)); }
				catch (...) { id = 0; }
				if (!merge::ForceWindow(id)) { extra = "false,\"error\":\"ref is the hex form id of a reference, and QuickLoot IE must be connected\""; }
			}
			else if (op == "unfocus") { merge::ClearForcedWindow(); }
			else if (op == "set")
			{
				if (!SetByName(Get(args, "name"), Get(args, "value"))) { extra = "false,\"error\":\"unknown name or bad value\""; }
				else { settings::Save(); }
			}
			else if (op == "restore")
			{
				settings::RestoreDefaults();
				settings::Save();
			}
			else if (op == "reload") { settings::Load(); }
			const std::string out = std::format(R"({{"ok":{},"op":"{}","ini":"{}",{}}})", extra, op.empty() ? "state" : op, "QuickLootContainerMerge.ini", merge::StateJson());
			a_write(a_sink, out.c_str());
		}
	}

	void Init(bool a_lastAttempt)
	{
		static bool registered = false;
		if (registered) { return; }
		DevBenchAPI::IDevBenchInterface001* devBench = DevBenchAPI::GetDevBenchInterface001();
		if (!devBench)
		{
			if (a_lastAttempt) { logger::info("DevBench not detected; skipping the \"qcm.control\" tool"); }
			return;
		}
		constexpr const char* descriptor =
			"{"
			"\"description\":\"QuickLoot Container Merge live state and driver. op=state (default): settings, QuickLoot IE's connection, what the last list merged (each source's form id, name, rows and red rows), how many references were left out, scan time, counters. "
			"op=action name=take|takeall|use|up|down|transfer: QuickLoot IE's own input action. op=focus ref=<hex form id>: opens the window on that reference without the crosshair. op=unfocus: back to the crosshair. op=set name=<INI key> value=<v>: sets and saves. op=restore: defaults, saved. op=reload: re-reads the INI.\","
			"\"inputSchema\":{\"type\":\"object\",\"properties\":{\"op\":{\"type\":\"string\"},\"ref\":{\"type\":\"string\"},\"name\":{\"type\":\"string\"},\"value\":{\"type\":\"string\"}}},"
			"\"readOnly\":false"
			"}";
		if (devBench->RegisterTool("qcm.control", descriptor, &ControlTool, nullptr))
		{
			logger::info("registered \"qcm.control\" with DevBench (build {})", devBench->GetBuildNumber());
			registered = true;
		}
	}
}
