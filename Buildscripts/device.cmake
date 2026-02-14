# FlxOS Device Profile Selection
#
# Provides utilities for device-specific component configuration
# This is a placeholder for future device profile expansion

function(flx_select_device DEVICE_NAME)
    message(STATUS "FlxOS: Device profile '${DEVICE_NAME}' selected")
    
    # Future: Add device-specific component inclusion logic here
    # For now, device profiles are handled via Kconfig
endfunction()

# Auto-detect device profile from Kconfig if available
function(flx_auto_detect_device)
    # This will be expanded in Phase 4 (Device Profile System)
    # For now, rely on CONFIG_FLXOS_DEVICE_* from Kconfig
    message(STATUS "FlxOS: Using Kconfig-based device profile")
endfunction()
