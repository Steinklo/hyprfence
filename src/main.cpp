#include <hyprland/src/version.h>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/managers/PointerManager.hpp>
#include <hyprland/src/plugins/HookSystem.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include <algorithm>

static HANDLE           PHANDLE     = nullptr;
static CFunctionHook*   g_pMoveHook = nullptr;
static bool             g_bypass    = false;
static bool             g_enabled   = true;

using origMoveT = void (*)(void*, const Vector2D&);

static bool isEnabled() {
    return PHANDLE && g_enabled;
}

// Re-reads plugin:hyprfence:enabled from the live config. Hooked to the
// config.reloaded event (see PLUGIN_INIT) rather than read once at startup:
// HyprlandAPI::reloadConfig() only queues a reload (see PluginAPI.hpp), so a
// synchronous read immediately after it still sees the pre-reload default —
// which is why the fence used to always start enabled regardless of the
// config file. Hooking the event also keeps g_enabled in sync with later
// reloads (e.g. `hyprctl reload` after a nix-rebuild), which a one-shot read
// at init could never do since PLUGIN_INIT doesn't run again.
static void syncEnabledFromConfig() {
    auto* val = HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprfence:enabled");
    if (!val)
        return;
    try {
        g_enabled = std::any_cast<Hyprlang::INT>(val->getValue()) != 0;
    } catch (...) {}
}

// RAII guard to prevent re-entrancy when warping
struct BypassGuard {
    BypassGuard()  { g_bypass = true; }
    ~BypassGuard() { g_bypass = false; }
};

// Hooks CPointerManager::move(const Vector2D& deltaLogical)
static void hkMove(void* thisptr, const Vector2D& deltaLogical) {
    PHLMONITOR mon = g_pCompositor->getMonitorFromCursor();

    reinterpret_cast<origMoveT>(g_pMoveHook->m_original)(thisptr, deltaLogical);

    if (g_bypass || !isEnabled() || !mon)
        return;

    const auto pos = g_pPointerManager->position();
    const auto box = mon->logicalBox();

    const auto clampedX = std::clamp(pos.x, box.x, box.x + box.w - 1.0);
    const auto clampedY = std::clamp(pos.y, box.y, box.y + box.h - 1.0);

    if (clampedX != pos.x || clampedY != pos.y) {
        BypassGuard guard;
        g_pPointerManager->warpTo(Vector2D(clampedX, clampedY));
    }
}

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH        = __hyprland_api_get_hash();
    const std::string CLIENT_HASH = __hyprland_api_get_client_hash();
    if (HASH != CLIENT_HASH) {
        HyprlandAPI::addNotification(PHANDLE,
            "[hyprfence] ABI mismatch! Host: " + HASH + " Plugin: " + CLIENT_HASH,
            CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hyprfence] Version mismatch");
    }

    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprfence:enabled", Hyprlang::INT{1});

    // Listener is static (lives for the process), so no need to hold the
    // CHyprSignalListener this would otherwise return.
    Event::bus()->m_events.config.reloaded.listenStatic(&syncEnabledFromConfig);

    // The plugin's config key wasn't registered yet during Hyprland's first
    // parse of the config file, so queue a reload now to pick it up; the
    // listener above applies the result once it actually runs.
    HyprlandAPI::reloadConfig();

    // Hook CPointerManager::move to confine cursor to current monitor
    auto fns = HyprlandAPI::findFunctionsByName(PHANDLE, "move");
    for (auto& fn : fns) {
        if (fn.demangled.contains("CPointerManager")) {
            g_pMoveHook = HyprlandAPI::createFunctionHook(PHANDLE, fn.address, (void*)&hkMove);
            break;
        }
    }

    if (!g_pMoveHook) {
        HyprlandAPI::addNotification(PHANDLE, "[hyprfence] Failed to hook move()!",
                                     CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hyprfence] Hook creation failed");
    }

    if (!g_pMoveHook->hook()) {
        HyprlandAPI::addNotification(PHANDLE, "[hyprfence] Failed to activate hook!",
                                     CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hyprfence] Hook activation failed");
    }

    // Dispatcher to toggle fence on/off at runtime
    HyprlandAPI::addDispatcherV2(PHANDLE, "hyprfence:toggle", [](std::string) -> SDispatchResult {
        g_enabled = !g_enabled;
        HyprlandAPI::addNotification(PHANDLE,
            g_enabled ? "[hyprfence] Enabled" : "[hyprfence] Disabled",
            g_enabled ? CHyprColor{0.2, 1.0, 0.2, 1.0} : CHyprColor{1.0, 0.6, 0.2, 1.0}, 2000);
        return {};
    });

    return {"hyprfence", "Confines cursor to the active monitor", "steinklo", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    if (g_pMoveHook) {
        g_pMoveHook->unhook();
        g_pMoveHook = nullptr;
    }
    PHANDLE = nullptr;
}
