#include "CoHModSDK.hpp"

#include <cstddef>

namespace {
    constexpr const char* kVehicleDecoratorUpdatePattern =
        "55 8B EC 83 E4 F8 6A FF 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 "
        "83 EC 24 53 55 56 57 8B E9 8B 85 80 00 00 00 85 C0 74 41 8B 48 54 85 C9 74 3A";

    constexpr std::ptrdiff_t kVehicleHealthWidgetOffset = 0x88;
    constexpr std::size_t kWidgetSetVisibleVTableIndex = 1;

    using VehicleDecoratorUpdateFn = void(__thiscall*)(void*);
    using WidgetSetVisibleFn = void(__thiscall*)(void*, int);

    VehicleDecoratorUpdateFn oFnVehicleDecoratorUpdate = nullptr;

    void SetWidgetVisible(void* widget, bool visible) {
        if (!widget) {
            return;
        }

        auto* const vtable = *reinterpret_cast<void***>(widget);
        if (!vtable || !vtable[kWidgetSetVisibleVTableIndex]) {
            return;
        }

        auto* const setVisible = reinterpret_cast<WidgetSetVisibleFn>(vtable[kWidgetSetVisibleVTableIndex]);
        setVisible(widget, visible ? 1 : 0);
    }

    void __fastcall HookedVehicleDecoratorUpdate(void* _this, void* edx) {
        oFnVehicleDecoratorUpdate(_this);

        auto* const vehicleHealthWidget = reinterpret_cast<void*>(reinterpret_cast<std::byte*>(_this) + kVehicleHealthWidgetOffset);
        SetWidgetVisible(vehicleHealthWidget, true);
    }

    bool SetupHook() {
        const auto tVehicleDecoratorUpdateResult = ModSDK::Memory::FindPattern("WW2Mod.dll", kVehicleDecoratorUpdatePattern);
        if (!tVehicleDecoratorUpdateResult.has_value()) {
			ModSDK::Dialogs::ShowError("Failed to find VehicleDecorator::Update");
            return false;
        }

		const auto tVehicleDecoratorUpdate = reinterpret_cast<void*>(tVehicleDecoratorUpdateResult.value());
        if (!ModSDK::Hooks::CreateHook(
                tVehicleDecoratorUpdate,
                reinterpret_cast<void*>(&HookedVehicleDecoratorUpdate),
                reinterpret_cast<void**>(&oFnVehicleDecoratorUpdate))) {
			ModSDK::Dialogs::ShowError("Failed to create VehicleDecorator::Update hook");
            return false;
        }

        if (!ModSDK::Hooks::EnableHook(tVehicleDecoratorUpdate)) {
			ModSDK::Dialogs::ShowError("Failed to enable VehicleDecorator::Update hook");
            return false;
        }

        return true;
    }

    bool OnInitialize() {
        return true;
    }

    bool OnModsLoaded() {
        return SetupHook();
    }

    void OnShutdown() {}

    const CoHModSDKModuleV1 kModule = {
        .abiVersion = COHMODSDK_ABI_VERSION,
        .size = sizeof(CoHModSDKModuleV1),
        .modId = "de.tosox.vehiclehpbars",
        .name = "Persistent Vehicle HP Bars",
        .version = "1.1.0",
        .author = "Tosox",
        .OnInitialize = &OnInitialize,
        .OnModsLoaded = &OnModsLoaded,
        .OnShutdown = &OnShutdown
    };
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    DisableThreadLibraryCalls(hModule);
    return TRUE;
}

COHMODSDK_EXPORT_MODULE(kModule);
