/**
 * @file Config.hpp
 * @brief Placeholder that triggers a build error when no device profile is selected.
 *
 * This file exists in the "no-profile" directory and is only included when
 * no valid device profile has been selected in menuconfig.
 */
#error "No device profile selected! Run 'idf.py menuconfig' and select a profile under: Hardware Configuration -> Device Profile"
