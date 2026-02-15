#pragma once

/**
 * @file Compat.hpp
 * @brief Namespace compatibility layer for incremental migration
 * 
 * This file provides namespace aliases to allow existing code using System::
 * to work with the new flx:: namespace during the migration period.
 * 
 * This file will be removed once all code is migrated to flx::.
 */

#include <flx/core/ClipboardManager.hpp>
#include <flx/core/Logger.hpp>
#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include <flx/core/Types.hpp>

// Namespace alias for backward compatibility
namespace flx::system {
// Import flx types into System namespace
using flx::ClipboardEntry;
using flx::ClipboardManager;
using flx::ClipboardOp;
using flx::Observable;
using flx::Result;
using flx::Singleton;
using flx::StringObservable;
} // namespace flx::system
