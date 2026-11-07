#include "PCH.h"

#include "SKSEMenuFramework.h"
#include "utils/PreciseSlider.h"
#include "utils/Toggle.h"

#include "Page.h"

#include "Merge.h"
#include "Settings.h"
#include "Strings.h"
#include "utils/Logger.h"

#include <atomic>
#include <mutex>

namespace page
{
	namespace
	{
		std::mutex g_statusLock;
		std::string g_status;
		std::atomic<bool> g_saveWanted{ false };

		void SetStatus(std::string a_text)
		{
			std::scoped_lock l(g_statusLock);
			g_status = std::move(a_text);
		}

		std::string Status()
		{
			std::scoped_lock l(g_statusLock);
			return g_status;
		}

		bool HasRequiredExports()
		{
			// Every export this page, Toggle.h and PreciseSlider.h resolve; the header does not null-check.
			constexpr const char* required[] = {
				"AddSectionItem", "igTextV", "igTextDisabledV", "igTextWrappedV", "igSetTooltipV", "igSliderInt", "igSliderFloat",
				"igIsItemHovered", "igButton", "igSameLine", "igSpacing", "igPushItemWidth", "igPopItemWidth", "igIsAnyItemActive", "igIsKeyDown_Nil",
				"igIsMouseDown_Nil", "igGetCursorScreenPos", "igGetWindowDrawList", "igGetFrameHeight", "igInvisibleButton", "igPushID_Str", "igPopID",
				"ImDrawList_AddRectFilled", "ImDrawList_AddCircleFilled"
			};
			for (const char* name : required)
			{
				if (!GetMenuFrameworkFunction<void*>(name))
				{
					logger::warn("the menu framework does not export \"{}\"; the settings page is not registered (the INI still works)", name);
					return false;
				}
			}
			return true;
		}

		void Help(const std::string& a_text)
		{
			ImGuiMCP::SameLine();
			ImGuiMCP::TextDisabled("%s", TR("QCM_HelpMark", "(?)").c_str());
			if (ImGuiMCP::IsItemHovered()) { ImGuiMCP::SetTooltip("%s", a_text.c_str()); }
		}

		bool ToggleRow(const std::string& a_label, std::atomic<bool>& a_value, const std::string& a_help)
		{
			bool v = a_value.load();
			const bool changed = ImGuiMCP::Toggle(a_label.c_str(), &v);
			if (changed) { a_value = v; }
			Help(a_help);
			return changed;
		}

		std::string ApiLine()
		{
			switch (merge::Api())
			{
			case merge::ApiState::kConnected: return TR("QCM_ApiConnected", "QuickLoot IE: connected.");
			case merge::ApiState::kTooOld: return TR("QCM_ApiTooOld", "QuickLoot IE: this version is too old. Update it to 4.1 or newer; nothing is merged until then.");
			default: return TR("QCM_ApiMissing", "QuickLoot IE: not installed. This mod does nothing without it.");
			}
		}

		void __stdcall Render()
		{
			using namespace ImGuiMCP;
			bool changed = false;

			TextWrapped("%s", TR("QCM_PageIntro", "Changes apply the next time the loot window opens and are saved to the mod's INI.").c_str());
			TextDisabled("%s", ApiLine().c_str());
			Spacing();

			changed |= ToggleRow(TR("QCM_Enabled", "Enabled"), settings::enabled, TR("QCM_HelpEnabled", "Off: the loot window shows only what the crosshair is on, as QuickLoot IE does alone."));

			PushItemWidth(260.0f);
			float interior = settings::interiorFeet.load();
			if (precise::SliderFloat(TR("QCM_InteriorFeet", "Indoors, feet").c_str(), &interior, settings::kFeetMin, settings::kFeetMax, "%.0f"))
			{
				settings::interiorFeet = interior;
				changed = true;
			}
			Help(TR("QCM_HelpInteriorFeet", "How far around you to look for others of the same kind, indoors."));
			float exterior = settings::exteriorFeet.load();
			if (precise::SliderFloat(TR("QCM_ExteriorFeet", "Outdoors, feet").c_str(), &exterior, settings::kFeetMin, settings::kFeetMax, "%.0f"))
			{
				settings::exteriorFeet = exterior;
				changed = true;
			}
			Help(TR("QCM_HelpExteriorFeet", "How far around you to look for others of the same kind, outdoors."));
			PopItemWidth();

			changed |= ToggleRow(TR("QCM_LineOfSight", "Only what you can see"), settings::lineOfSight,
				TR("QCM_HelpLineOfSight", "On: a container or body you have no line of sight to, such as one behind a wall, stays out of the list."));
			changed |= ToggleRow(TR("QCM_MixKinds", "Mix bodies and containers"), settings::mixKinds,
				TR("QCM_HelpMixKinds", "On: a body also lists the containers nearby, and a container the bodies, in one list. Locked containers, merchant chests, your own storage and summoned creatures still stay out. Off: bodies with bodies, containers with containers."));
			changed |= ToggleRow(TR("QCM_IncludeStealing", "Include items you'd have to steal"), settings::includeStealing,
				TR("QCM_HelpIncludeStealing", "On: nearby items that belong to someone else are listed in red with Steal, and taking them is a crime. Off: they are left out of the list. What the crosshair is on is shown as QuickLoot IE shows it."));

			Spacing();
			if (Button(TR("QCM_Restore", "Restore defaults").c_str()))
			{
				settings::RestoreDefaults();
				changed = true;
				SetStatus(TR("QCM_StatusRestored", "Defaults restored and saved."));
			}
			else if (changed)
			{
				SetStatus(TR("QCM_StatusSaved", "Saved."));
			}

			if (changed) { g_saveWanted = true; }
			// One write per change: wait until no control is held (a slider being dragged changes every frame).
			if (g_saveWanted.load() && !IsAnyItemActive())
			{
				g_saveWanted = false;
				if (auto* tasks = SKSE::GetTaskInterface()) { tasks->AddTask([] { settings::Save(); }); }
			}
			if (const auto status = Status(); !status.empty()) { TextDisabled("%s", status.c_str()); }
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled() || !GetMenuFrameworkModule())
		{
			logger::info("no menu framework is loaded; settings are read from the INI only");
			return;
		}
		if (!HasRequiredExports()) { return; }
		SKSEMenuFramework::SetSection("QuickLoot Container Merge");
		SKSEMenuFramework::AddSectionItem("Settings", Render);
		logger::info("settings page registered");
	}
}
