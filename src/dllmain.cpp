#include "CoHModSDK.hpp"

#include <cstddef>

namespace {
    constexpr const char* kVehicleDecoratorUpdatePattern =
        "55 8B EC 83 E4 F8 6A FF 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 "
        "83 EC 24 53 55 56 57 8B E9 8B 85 80 00 00 00 85 C0 74 41 8B 48 54 85 C9 74 3A";
    constexpr const char* kVehicleDecoratorVeterancyHidePattern =
        "53 EB 08 8D 8D 88 00 00 00 6A 01 8B 01 8B 50 04 FF D2 89 5C 24 20 89 5C 24 24";

    constexpr std::ptrdiff_t kVehicleHealthWidgetOffset = 0x88;
    constexpr std::size_t kWidgetSetVisibleVTableIndex = 1;
    constexpr unsigned char kVehicleDecoratorVeterancyHideOriginalBytes[] = { 0x53, 0xEB, 0x08 };
    constexpr unsigned char kVehicleDecoratorVeterancyHidePatchedBytes[] = { 0x90, 0xEB, 0x0F };

    using VehicleDecoratorUpdateFn = void(__thiscall*)(void*);
    using WidgetSetVisibleFn = void(__thiscall*)(void*, bool);

    VehicleDecoratorUpdateFn oFnVehicleDecoratorUpdate = nullptr;
    void* tVehicleDecoratorVeterancyHide = nullptr;

    void SetWidgetVisible(void* widget, bool visible) {
        if (!widget) {
            return;
        }

        auto* const vtable = *reinterpret_cast<void***>(widget);
        if (!vtable || !vtable[kWidgetSetVisibleVTableIndex]) {
            return;
        }

        auto* const setVisible = reinterpret_cast<WidgetSetVisibleFn>(vtable[kWidgetSetVisibleVTableIndex]);
        setVisible(widget, visible);
    }

    void __fastcall HookedVehicleDecoratorUpdate(void* _this, void* edx) {
        oFnVehicleDecoratorUpdate(_this);

        auto* const vehicleHealthWidget = reinterpret_cast<void*>(reinterpret_cast<std::byte*>(_this) + kVehicleHealthWidgetOffset);
        SetWidgetVisible(vehicleHealthWidget, true);
    }

    bool SetupHook() {
        const auto tVehicleDecoratorVeterancyHideResult = ModSDK::Memory::FindPattern("WW2Mod.dll", kVehicleDecoratorVeterancyHidePattern);
        if (!tVehicleDecoratorVeterancyHideResult.has_value()) {
            ModSDK::Dialogs::ShowError("Failed to find VehicleDecorator::Update (veterancy hide site)");
            return false;
        }

        tVehicleDecoratorVeterancyHide = reinterpret_cast<void*>(tVehicleDecoratorVeterancyHideResult.value());
        ModSDK::Memory::PatchMemory(
            tVehicleDecoratorVeterancyHide,
            kVehicleDecoratorVeterancyHidePatchedBytes,
            sizeof(kVehicleDecoratorVeterancyHidePatchedBytes)
        );

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

        return true;
    }

    bool OnInitialize() {
        return SetupHook();
    }

    void OnShutdown() {
		if (tVehicleDecoratorVeterancyHide != nullptr) {
            ModSDK::Memory::PatchMemory(
                tVehicleDecoratorVeterancyHide,
                kVehicleDecoratorVeterancyHideOriginalBytes,
                sizeof(kVehicleDecoratorVeterancyHideOriginalBytes)
            );
        }
    }

    const CoHModSDKModuleV1 kModule = {
        .abiVersion = COHMODSDK_ABI_VERSION,
        .size = sizeof(CoHModSDKModuleV1),
        .modId = "de.tosox.persistentvehiclehpbars",
        .name = "Persistent Vehicle HP Bars",
        .version = "1.2.0",
        .author = "Tosox",
        .OnInitialize = &OnInitialize,
        .OnShutdown = &OnShutdown
    };
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    DisableThreadLibraryCalls(hModule);
    return TRUE;
}

COHMODSDK_EXPORT_MODULE(kModule);
