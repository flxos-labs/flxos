# FlxOS Ultimate Wallpaper Engine Plan

**Status**: Design Phase  
**Version**: 1.1  
**Last Updated**: March 25, 2026  
**Target Platform**: ESP32, ESP32-S3, ESP32-P4  

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Current State Analysis](#current-state-analysis)
3. [LVGL Capabilities](#lvgl-capabilities)
4. [Architecture Design](#architecture-design)
5. [Wallpaper Engine App](#wallpaper-engine-app)
6. [Component Specifications](#component-specifications)
7. [Implementation Roadmap](#implementation-roadmap)
8. [Technical Considerations](#technical-considerations)
9. [Storage & File Structure](#storage--file-structure)
10. [API Reference](#api-reference)
11. [Risk Assessment](#risk-assessment)
12. [Hard-Part De-Risk Plan](#hard-part-de-risk-plan)
13. [LVGL and FreeRTOS Concurrency Model](#lvgl-and-freertos-concurrency-model)
14. [Preset Production Pipeline](#preset-production-pipeline)

---

## Executive Summary

### Vision
Transform FlxOS wallpaper system from a basic static image viewer into a professional **wallpaper engine** supporting:
- ✨ Animated wallpapers (GIF, Lottie, custom animations)
- 🎨 Visual effects (blur, brightness, parallax, transitions)
- 🔮 Dynamic/generative wallpapers (algorithmic content)
- 📱 Adaptive wallpapers (time, weather, battery-aware)
- 📚 Preset library with community sharing
- ⚡ Performance-optimized for embedded systems

### Expected Outcomes
- **Desktop Polish**: Professional wallpaper experience rivaling mobile OS
- **User Personalization**: Rich customization options
- **Community Engagement**: Wallpaper preset marketplace potential
- **System Differentiation**: Advanced wallpaper features unique to FlxOS

---

## Current State Analysis

### Existing Wallpaper System

**What's Implemented:**
```
System Layer (ThemeManager)
    ↓ Observable properties
UI Bridge (UiThemeManager)
    ↓ LVGL synchronization
Desktop Shell (Desktop.cpp)
    ↓ LVGL Image widget rendering
```

**Capabilities:**
- ✅ Static image support (PNG, JPG, BMP)
- ✅ File browser selection
- ✅ Persistent settings storage
- ✅ LVGL image caching
- ✅ Theme-based color fallback
- ✅ CoverMode aspect ratio preservation

**Limitations:**
- ❌ No animation support
- ❌ Single static image only
- ❌ No visual effects (blur, brightness, etc.)
- ❌ No parallax or multi-layer backgrounds
- ❌ No dynamic/generative wallpapers
- ❌ No transitions between wallpapers
- ❌ No performance optimization for animation
- ❌ No preset library or defaults
- ❌ No aspect ratio options (only cover mode)
- ❌ No wallpaper preview system

### Architecture Diagram

```
┌──────────────────────────────────┐
│  Settings (user selects wallpaper)│
└────────────┬─────────────────────┘
             │
┌────────────▼─────────────────────┐
│  ThemeManager (System/)           │
│  - wp_enabled (Observable)        │
│  - wp_path (StringObservable)     │
│  - Settings persistence           │
└────────────┬─────────────────────┘
             │
┌────────────▼─────────────────────┐
│  UiThemeManager (UI/)             │
│  - LVGL bridge                    │
│  - Observer synchronization       │
└────────────┬─────────────────────┘
             │
┌────────────▼─────────────────────┐
│  Desktop (UI/Source/desktop/)     │
│  - m_wallpaper container          │
│  - m_wallpaper_img (lv_image)     │
│  - createWallpaperImage()         │
└────────────┬─────────────────────┘
             │
┌────────────▼─────────────────────┐
│  LVGL Rendering Engine            │
└──────────────────────────────────┘
```

### Settings Storage
```
Settings (NVS or SPIFFS):
  ├─ wp_enabled: 1/0
  ├─ wp_path: "/path/to/wallpaper.png"
  ├─ wp_type: "static|animated|lottie|dynamic|adaptive"
  └─ wp_effects: JSON string with effect config
```

---

## LVGL Capabilities

### Animation Support (LVGL v9.5)

**1. Image Sequences (AnimImage)**
```cpp
lv_animimage_create()           // Create animation widget
lv_animimage_set_src()          // Set frame array
lv_animimage_set_duration()     // Play duration
lv_animimage_set_repeat_count() // Loop vs play-once
```
- **Use Case**: GIF-like wallpapers, frame sequences
- **Frame Support**: 24-bit RGB, 8-bit indexed color
- **Format**: PNG/JPG frames or sprite sheets
- **Performance**: Low overhead, LVGL managed

**2. Vector Animation (Lottie)**
```cpp
// Via ThorVG integration (built-in LVGL)
lv_obj_set_style_bg_img_src()   // Set Lottie JSON
// SVG/Lottie playback handled by thorvg library
```
- **Use Case**: Smooth vector animations, small filesize
- **Format**: JSON (Lottie format or raw SVG)
- **Scalability**: Resolution-independent
- **Size Benefits**: ~10-50KB for complex animations vs 1MB+ GIF

**3. Canvas Drawing (Custom Rendering)**
```cpp
lv_canvas_create()                     // Get pixel buffer
canvas_buf = lv_canvas_get_buf()       // Direct pixel access
// Render custom content per-frame
lv_canvas_finish_layer()               // Upload to display
```
- **Use Case**: Generative wallpapers, effects processing
- **Limitations**: CPU-intensive, not real-time at full resolution
- **Optimization**: Draw to smaller buffer, scale up
- **Examples**: Plasma, Perlin noise, particle systems, fractals

**4. Timeline-based Animation**
```cpp
lv_obj_add_timeline()                  // Create animation track
lv_timeline_add_task()                 // Add animation event
// Keyframe-based property changes (position, size, color, opacity)
```
- **Use Case**: Smooth transitions, fade-ins, property morphing
- **Features**: Easing functions, parallel/sequential playback

### Image Format Support

| Format | Support | LVGL Library | Notes |
|--------|---------|--------------|-------|
| PNG | ✅ Core | libpng/lodepng | Lossless, universal |
| JPEG | ✅ Core | libjpeg_turbo | Lossy compression |
| BMP | ✅ Core | Built-in | Uncompressed, large |
| GIF | ✅ AnimImage | giflib | Animation support |
| WebP | ✅ High-quality | libwebp | Modern compression |
| SVG | ✅ Vector | thorvg | Scalable graphics |
| JSON (Lottie) | ✅ Vector | thorvg/rlottie | Animation framework |

### Performance Features

**Built-in Optimization:**
- **Image Cache**: `lv_image_cache*` functions for memory efficiency
- **Hardware Acceleration**: 
  - ESP32-S3: DMA2D support (via driver layer)
  - ESP32-P4: PPA (Pixel Processing Accelerator)
- **Draw System**: Tile-based rendering, partial screen updates
- **Memory Allocator**: TLSF or configurable malloc

**Display Driver Integration:**
```cpp
// Fast image loading via SPI/QSPI
// Direct framebuffer rendering
// Partial screen updates (dirty region tracking)
```

---

## Architecture Design

### System Overview

```
┌─────────────────────────────────────────────────┐
│           Wallpaper Engine Core                 │
├─────────────────────────────────────────────────┤
│ WallpaperManager (Service)                      │
│ ├─ AnimationController                          │
│ ├─ EffectPipeline                               │
│ ├─ PresetLibrary                                │
│ └─ PerformanceMonitor                           │
├─────────────────────────────────────────────────┤
│ Wallpaper Providers (Plugin Architecture)       │
│ ├─ IWallpaperProvider (Interface)               │
│ ├─ StaticImageProvider                          │
│ ├─ AnimatedGifProvider                          │
│ ├─ LottieProvider                               │
│ ├─ DynamicProvider (Generative)                 │
│ ├─ AdaptiveProvider (Time/Weather)              │
│ └─ VideoProvider (Optional Future)              │
├─────────────────────────────────────────────────┤
│ Settings & Persistence                          │
│ ├─ WallpaperSettings (Observable patterns)      │
│ ├─ PresetConfiguration                          │
│ ├─ EffectConfiguration                          │
│ └─ SettingsObservers                            │
├─────────────────────────────────────────────────┤
│ UI Integration                                  │
│ ├─ DisplaySettings (Controls)                   │
│ ├─ PresetGallery (Browser)                      │
│ └─ WallpaperPreview                             │
├─────────────────────────────────────────────────┤
│ LVGL Rendering Layer                            │
│ └─ WallpaperWidget (Main rendering object)     │
└─────────────────────────────────────────────────┘
```

### Layer Responsibilities

| Layer | Responsibility | Files |
|-------|----------------|-------|
| **Core** | Service management, orchestration | `System/Source/managers/WallpaperManager.*` |
| **Provider** | Different wallpaper types | `UI/Source/wallpaper/providers/*.cpp` |
| **Effects** | Visual processing pipeline | `UI/Source/wallpaper/effects/*.cpp` |
| **UI** | User interaction, settings | `Applications/settings/wallpaper/*` |
| **LVGL** | Low-level rendering | `UI/Source/wallpaper/widget/*` |
| **Storage** | Persistence, file I/O | `/data/wallpapers/**` |

---

## Wallpaper Engine App

### App Scope and Role

The Wallpaper Engine App is a user-facing control surface for wallpaper features. It does not own rendering or provider lifecycles directly. Runtime ownership remains in `WallpaperManager`.

**Separation of Responsibilities:**
- **WallpaperManager (Engine Core):** provider lifecycle, rendering state, quality adaptation, settings sync, fallback logic.
- **Wallpaper Engine App (UI Shell):** browsing presets, previewing changes, editing effect parameters, applying or scheduling wallpaper profiles.

### UI/Engine Contract

The app communicates through manager APIs and observables only:
- Read current state (`type`, `source`, `effects`, `speed`, `quality`, perf metrics).
- Write intent (`setWallpaper`, `setAnimationSpeed`, `setQualityLevel`, `apply/remove effect`).
- Subscribe to events (`wallpaper.changed`, `wallpaper.error`, fallback notifications).

The app must not call LVGL wallpaper provider internals directly.

### Proposed App Placement

```
Applications/
└── wallpaper_engine/
    ├── WallpaperEngineApp.cpp
    ├── WallpaperEngineApp.hpp
    ├── pages/
    │   ├── PresetsPage.cpp
    │   ├── EffectsPage.cpp
    │   ├── DynamicPage.cpp
    │   └── AdaptivePage.cpp
    └── widgets/
        ├── WallpaperPreviewCard.cpp
        └── EffectSliderRow.cpp
```

### MVP Feature Set

- Preset grid with thumbnail preview and one-tap apply.
- Effect controls (blur, brightness, overlay) with live preview.
- Animation controls (speed, loop mode where applicable).
- Safe fallback banner when current wallpaper fails and engine downgrades.

### Non-Goals (Phase 1)

- No direct decoder ownership in app.
- No separate background renderer inside app.
- No duplicated wallpaper state outside manager observables.

---

## Component Specifications

### 1. WallpaperManager Service

**Location**: `System/Include/flx/core/managers/WallpaperManager.hpp`

**Responsibilities:**
- Main service coordinating all wallpaper operations
- Lifecycle management (init, start, stop, destroy)
- Observable property management
- Provider instantiation and switching
- Effect pipeline management
- Performance monitoring

**Key Properties (Observables):**
```cpp
class WallpaperManager : public IService {
    // Configuration
    Observable<int32_t> m_wallpaper_enabled;       // 0=disabled, 1=enabled
    StringObservable m_wallpaper_type;             // "static"|"animated"|"lottie"|"dynamic"|"adaptive"
    StringObservable m_wallpaper_source;           // File path, preset ID, URL
    StringObservable m_wallpaper_effects;          // JSON config: "blur":5, "brightness":1.0, etc
    Observable<int32_t> m_animation_speed;        // 0-100 (percent)
    Observable<int32_t> m_quality_level;          // 0=low, 1=medium, 2=high
    Observable<float> m_cpu_usage;                // FPS feedback for UI
    
    // Methods
    bool onStart() override;
    void onStop() override;
    void onFrame(uint32_t delta_ms);              // Called by event loop
    void setWallpaper(const std::string& source, const std::string& type);
    void applyEffect(const std::string& effect_name, const Json::Value& params);
    void removeEffect(const std::string& effect_name);
    void setAnimationSpeed(int32_t speed);
    void setQualityLevel(int32_t level);
    
    // Providers
    void switchProvider(const std::string& type);
    IWallpaperProvider* getCurrentProvider();
};
```

**Integration Points:**
- Registered as service in `ServiceRegistry`
- Observable subjects synced with `SettingsManager`
- Dependents: `Desktop`, `DisplaySettings` UI
- Events: `wallpaper.changed`, `wallpaper.effect_applied`, `wallpaper.error`

### 2. Provider Interface & Implementations

**Base Interface**: `UI/Include/flx/ui/wallpaper/IWallpaperProvider.hpp`

```cpp
class IWallpaperProvider {
public:
    virtual ~IWallpaperProvider() = default;
    
    // Lifecycle
    virtual void initialize() = 0;
    virtual void destroy() = 0;
    
    // Rendering
    virtual void render(lv_obj_t* parent, uint32_t elapsed_ms) = 0;
    
    // Configuration
    virtual void setSource(const std::string& source) = 0;
    virtual void setAnimationSpeed(int32_t speed) = 0;
    
    // State queries
    virtual bool isAnimated() const = 0;
    virtual bool isReady() const = 0;
    virtual std::string getType() const = 0;
    virtual size_t getMemoryUsage() const = 0;
    
    // Error handling
    virtual std::string getLastError() const = 0;
};
```

#### Provider 2A: StaticImageProvider

**File**: `UI/Source/wallpaper/providers/StaticImageProvider.cpp`

**Features:**
- Renders static images (PNG, JPG, BMP, WebP)
- Refactored from existing `Desktop::createWallpaperImage()`
- Uses LVGL image cache
- CoverMode aspect ratio scaling
- Minimal CPU usage

**Implementation:**
```cpp
class StaticImageProvider : public IWallpaperProvider {
    lv_obj_t* m_image_obj;
    std::string m_image_path;
    
    void render(lv_obj_t* parent, uint32_t elapsed_ms) override {
        if (!m_image_obj) {
            m_image_obj = lv_image_create(parent);
            lv_image_set_inner_align(m_image_obj, LV_IMAGE_ALIGN_COVER);
            lv_obj_set_size(m_image_obj, lv_pct(100), lv_pct(100));
        }
        // Image displayed, no per-frame updates needed
    }
    
    void setSource(const std::string& source) override {
        if (lv_image_cache_is_enabled()) {
            lv_image_cache_drop(m_image_path.c_str());
        }
        lv_image_set_src(m_image_obj, source.c_str());
        m_image_path = source;
    }
    
    bool isAnimated() const override { return false; }
};
```

#### Provider 2B: AnimatedGifProvider

**File**: `UI/Source/wallpaper/providers/AnimatedGifProvider.cpp`

**Features:**
- GIF wallpapers with frame-by-frame animation
- Uses LVGL's `lv_animimage` widget
- Configurable frame rate
- Loop detection and repeat count

**Implementation:**
```cpp
class AnimatedGifProvider : public IWallpaperProvider {
    lv_obj_t* m_anim_obj;
    std::vector<const void*> m_frames;
    int32_t m_animation_speed; // 0-100
    
    void initialize() override {
        m_animation_speed = 50; // Default 50%
    }
    
    void render(lv_obj_t* parent, uint32_t elapsed_ms) override {
        if (!m_anim_obj) {
            m_anim_obj = lv_animimage_create(parent);
            lv_obj_set_size(m_anim_obj, lv_pct(100), lv_pct(100));
        }
        // LVGL handles animation internally
    }
    
    void setSource(const std::string& source) override {
        loadGifFrames(source);
        lv_animimage_set_src(m_anim_obj, m_frames.data(), m_frames.size());
        updateFrameRate();
    }
    
    void setAnimationSpeed(int32_t speed) override {
        m_animation_speed = speed;
        updateFrameRate();
    }
    
    bool isAnimated() const override { return true; }
    
private:
    void updateFrameRate() {
        // Adjust animation duration based on speed (0-100%)
        uint32_t base_duration = 100;
        uint32_t adjusted = (base_duration * 100) / m_animation_speed;
        lv_animimage_set_duration(m_anim_obj, adjusted);
    }
};
```

#### Provider 2C: LottieProvider

**File**: `UI/Source/wallpaper/providers/LottieProvider.cpp`

**Features:**
- Vector animation support via ThorVG (built-in LVGL)
- SVG and Lottie JSON format
- Resolution-independent scaling
- Smaller filesize than GIF (~10-50KB vs 1MB+)

**Implementation:**
```cpp
class LottieProvider : public IWallpaperProvider {
    lv_obj_t* m_lottie_obj;
    std::string m_lottie_path;
    int32_t m_animation_speed;
    
    void render(lv_obj_t* parent, uint32_t elapsed_ms) override {
        if (!m_lottie_obj) {
            m_lottie_obj = lv_obj_create(parent);
            lv_obj_set_size(m_lottie_obj, lv_pct(100), lv_pct(100));
            lv_obj_set_style_bg_opa(m_lottie_obj, LV_OPA_TRANSP, 0);
        }
        // ThorVG handles rendering from JSON
    }
    
    void setSource(const std::string& source) override {
        m_lottie_path = source;
        // Load JSON and setup ThorVG rendering
        setupLottieRendering();
    }
    
    bool isAnimated() const override { return true; }
};
```

#### Provider 2D: DynamicProvider

**File**: `UI/Source/wallpaper/providers/DynamicProvider.cpp`

**Features:**
- Algorithmic/generative wallpapers
- Canvas-based custom pixel rendering
- CPU-intensive but visually impressive
- Configurable algorithms and parameters

**Supported Algorithms:**
1. **Plasma**: Sine wave interference pattern
2. **Perlin Noise**: Smooth random cloud-like pattern
3. **Particles**: Animated particle system
4. **Mandelbrot**: Fractal rendering (with bailout)
5. **Gradient Waves**: Dynamic color gradients with animation
6. **Noise Clouds**: Layered Perlin noise with color mapping

**Implementation:**
```cpp
class DynamicProvider : public IWallpaperProvider {
    lv_obj_t* m_canvas;
    uint8_t* m_canvas_buf;
    std::string m_algorithm;
    Json::Value m_algorithm_params;
    uint32_t m_elapsed_time;
    
    void render(lv_obj_t* parent, uint32_t elapsed_ms) override {
        if (!m_canvas) {
            m_canvas = lv_canvas_create(parent);
            lv_obj_set_size(m_canvas, lv_pct(100), lv_pct(100));
            m_canvas_buf = (uint8_t*)malloc(CANVAS_WIDTH * CANVAS_HEIGHT * 4);
            lv_canvas_set_buffer(m_canvas, m_canvas_buf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_COLOR_FORMAT_ARGB8888);
        }
        
        m_elapsed_time += elapsed_ms;
        
        // Render based on algorithm type
        if (m_algorithm == "plasma") {
            renderPlasma(elapsed_ms);
        } else if (m_algorithm == "perlin") {
            renderPerlinNoise(elapsed_ms);
        } else if (m_algorithm == "particles") {
            renderParticles(elapsed_ms);
        }
        
        lv_canvas_finish_layer(m_canvas);
    }
    
    void setSource(const std::string& source) override {
        // source format: "algo://algorithm_name?param1=value1&param2=value2"
        parseAlgorithmSource(source);
    }
    
    bool isAnimated() const override { return true; }
    
private:
    void renderPlasma(uint32_t elapsed_ms) {
        for (int y = 0; y < CANVAS_HEIGHT; y++) {
            for (int x = 0; x < CANVAS_WIDTH; x++) {
                float val = sin(x * 0.05 + elapsed_ms * 0.001);
                val += sin(y * 0.05 - elapsed_ms * 0.001);
                val += sin((x + y) * 0.02 + elapsed_ms * 0.0005);
                
                uint32_t color = hsvToRgb((val * 90) % 360, 255, 200);
                drawPixel(x, y, color);
            }
        }
    }
    
    void renderPerlinNoise(uint32_t elapsed_ms) {
        // Layered Perlin noise with time parameter
        // Smooth cloud-like pattern
    }
    
    void renderParticles(uint32_t elapsed_ms) {
        // Particle system with gravity/physics
        // Colorful animated particles
    }
};
```

#### Provider 2E: AdaptiveProvider

**File**: `UI/Source/wallpaper/providers/AdaptiveProvider.cpp`

**Features:**
- Time-of-day aware wallpapers
- Weather integration
- Battery level responsive
- System load adaptive

**Implementation:**
```cpp
class AdaptiveProvider : public IWallpaperProvider {
    enum AdaptiveMode {
        TIME_OF_DAY,
        WEATHER_BASED,
        BATTERY_LEVEL,
        SYSTEM_LOAD
    };
    
    AdaptiveMode m_mode;
    IWallpaperProvider* m_active_provider;
    
    void render(lv_obj_t* parent, uint32_t elapsed_ms) override {
        updateAdaptiveState();
        if (m_active_provider) {
            m_active_provider->render(parent, elapsed_ms);
        }
    }
    
private:
    void updateAdaptiveState() {
        if (m_mode == TIME_OF_DAY) {
            // Morning (6-12): Bright, warm wallpaper
            // Afternoon (12-18): Vibrant, energetic
            // Evening (18-23): Cool, calm
            // Night (23-6): Dark, minimal
            switchProviderBasedOnTime();
        } else if (m_mode == BATTERY_LEVEL) {
            // Low battery: Static image, no animation
            // Normal: Animated wallpaper
            // Charging: Effect-rich wallpaper
            updateBasedOnBattery();
        }
    }
};
```

### 3. Effect Pipeline

**File**: `UI/Include/flx/ui/wallpaper/EffectPipeline.hpp`

```cpp
class IEffect {
public:
    virtual ~IEffect() = default;
    virtual void process(lv_obj_t* target, uint32_t elapsed_ms) = 0;
    virtual void configure(const Json::Value& params) = 0;
    virtual std::string getName() const = 0;
};

class EffectPipeline {
private:
    std::map<std::string, std::unique_ptr<IEffect>> m_effects;
    
public:
    void addEffect(const std::string& name, std::unique_ptr<IEffect> effect) {
        m_effects[name] = std::move(effect);
    }
    
    void removeEffect(const std::string& name) {
        m_effects.erase(name);
    }
    
    void processFrame(lv_obj_t* target, uint32_t elapsed_ms) {
        for (auto& [name, effect] : m_effects) {
            effect->process(target, elapsed_ms);
        }
    }
};
```

**Built-in Effects:**

1. **BlurEffect**: Gaussian blur (radius: 0-20)
2. **BrightnessEffect**: Brightness/contrast adjustment
3. **ParallaxEffect**: Mouse/tilt-based parallax
4. **FadeTransitionEffect**: Fade between wallpapers
5. **ZoomEffect**: Subtle zoom animation
6. **ColorOverlayEffect**: Tinted overlay color
7. **VignetteEffect**: Darkened edges

### 4. PerformanceMonitor

**File**: `UI/Include/flx/ui/wallpaper/PerformanceMonitor.hpp`

```cpp
class PerformanceMonitor {
public:
    struct Metrics {
        float current_fps;
        float average_fps;
        float cpu_usage_percent;
        float memory_usage_bytes;
        uint32_t frame_time_ms;
    };
    
    void recordFrame(uint32_t frame_time_ms);
    Metrics getMetrics() const;
    
    bool shouldReduceQuality() const;
    int32_t getAdaptiveQualityLevel() const;
    
    // Thresholds
    static constexpr float TARGET_FPS = 30.0f;
    static constexpr float CPU_WARNING = 60.0f;  // Percent
    static constexpr float MEM_WARNING = 50.0f;  // Percent of available
};
```

### 5. PresetLibrary

**File**: `UI/Include/flx/ui/wallpaper/PresetLibrary.hpp`

```cpp
struct WallpaperPreset {
    std::string id;           // "sunset", "ocean", "forest"
    std::string name;         // Display name
    std::string description;
    std::string type;         // "static", "animated", "lottie", "dynamic"
    std::string source;       // File path or algorithm ID
    Json::Value effects;      // Effect configuration
    std::string thumbnail;    // Path to preview image
    bool is_builtin;          // Part of system
};

class PresetLibrary {
private:
    std::map<std::string, WallpaperPreset> m_presets;
    std::string m_presets_dir;
    
public:
    void loadPresets();
    const WallpaperPreset* getPreset(const std::string& id) const;
    std::vector<const WallpaperPreset*> listPresets() const;
    bool applyPreset(const std::string& id, WallpaperManager* manager);
    
    // User management
    bool saveUserPreset(const WallpaperPreset& preset);
    bool deleteUserPreset(const std::string& id);
};
```

---

## Implementation Roadmap

### Cross-Phase Track: Wallpaper Engine App (Weeks 2-10)

**Goal**: Deliver a dedicated app that controls the engine without duplicating engine logic.

**Tasks:**
- [ ] Create `WallpaperEngineApp` shell and navigation pages.
- [ ] Bind app controls to `WallpaperManager` observables and APIs.
- [ ] Add preset browser and preview card powered by `PresetLibrary` metadata.
- [ ] Add effect editor rows and animation controls.
- [ ] Add runtime error/fallback indicator sourced from wallpaper events.
- [ ] Add app-level integration tests for apply/rollback/fallback UX.

**Files to Create:**
- `Applications/wallpaper_engine/WallpaperEngineApp.cpp`
- `Applications/wallpaper_engine/WallpaperEngineApp.hpp`
- `Applications/wallpaper_engine/pages/*.cpp`
- `Applications/wallpaper_engine/widgets/*.cpp`

**Deliverable**: A standalone Wallpaper Engine App that acts as the primary UX for wallpaper features while `WallpaperManager` remains the engine runtime.

### Phase 1: Foundation (Weeks 1-2)

**Goal**: Core infrastructure and provider interface

**Tasks:**
- [ ] Create `WallpaperManager` service class skeleton
- [ ] Define `IWallpaperProvider` interface
- [ ] Implement `StaticImageProvider` (refactor existing code)
- [ ] Update `ServiceRegistry` to register `WallpaperManager`
- [ ] Create Observable properties for wallpaper settings
- [ ] Integrate `Desktop::onFrame()` callback to `WallpaperManager`

**Files to Create:**
- `System/Include/flx/core/managers/WallpaperManager.hpp`
- `System/Source/managers/WallpaperManager.cpp`
- `UI/Include/flx/ui/wallpaper/IWallpaperProvider.hpp`
- `UI/Source/wallpaper/providers/StaticImageProvider.cpp`

**Deliverable**: Wallpaper system works exactly as before, but refactored into engine

### Phase 2: Animation Support (Weeks 3-4)

**Goal**: AnimImage (GIF) and Lottie support

**Tasks:**
- [ ] Spike A: GIF decode path on device (max tested dimensions, frame count, decode time)
- [ ] Spike B: Lottie/ThorVG validation on target boards (heap use, frame time, fallback behavior)
- [ ] Implement `AnimatedGifProvider`
- [ ] Implement `LottieProvider`
- [ ] Add animation speed control (0-100%)
- [ ] Test with sample GIF and Lottie files
- [ ] Update Settings UI to select wallpaper type
- [ ] Add animation speed slider to Settings
- [ ] Add strict acceptance gates:
    - GIF: sustained >= 24 FPS for 240x320, <= 180KB extra heap
    - Lottie: sustained >= 24 FPS for "medium" complexity scenes, <= 220KB extra heap
    - Failure path: automatic downgrade to static wallpaper with warning event

**Files to Create:**
- `UI/Source/wallpaper/providers/AnimatedGifProvider.cpp`
- `UI/Source/wallpaper/providers/LottieProvider.cpp`

**Deliverable**: Users can select animated GIF or Lottie wallpapers with speed control, and the implementation is guarded by device-proven acceptance gates

### Phase 3: Effect System (Weeks 5-6)

**Goal**: Visual effects pipeline

**Tasks:**
- [ ] Create `EffectPipeline` architecture
- [ ] Implement `BlurEffect` (canvas-based)
- [ ] Implement `BrightnessEffect`
- [ ] Implement `FadeTransitionEffect`
- [ ] Update Settings UI with effect controls
- [ ] Create effect parameter UI components (sliders, toggles)

**Files to Create:**
- `UI/Include/flx/ui/wallpaper/EffectPipeline.hpp`
- `UI/Source/wallpaper/effects/BlurEffect.cpp`
- `UI/Source/wallpaper/effects/BrightnessEffect.cpp`
- `UI/Source/wallpaper/effects/FadeTransitionEffect.cpp`

**Deliverable**: Users can apply blur, brightness adjustments, and smooth transitions

### Phase 4: Dynamic Wallpapers (Weeks 7-8)

**Goal**: Generative algorithmic wallpapers

**Tasks:**
- [ ] Implement `DynamicProvider` with canvas rendering
- [ ] Implement Plasma algorithm
- [ ] Implement Perlin Noise algorithm
- [ ] Implement Gradient Waves
- [ ] Implement `PerformanceMonitor` for adaptive quality
- [ ] Add dynamic wallpaper selection to Settings
- [ ] Add algorithm parameter controls

**Files to Create:**
- `UI/Source/wallpaper/providers/DynamicProvider.cpp`
- `UI/Include/flx/ui/wallpaper/PerformanceMonitor.hpp`
- `UI/Source/wallpaper/PerformanceMonitor.cpp`

**Deliverable**: Impressive generative wallpapers with performance monitoring

### Phase 5: Preset Library (Weeks 9-10)

**Goal**: Built-in presets and preset management

**Tasks:**
- [ ] Create 10-15 beautiful presets:
  - 3 static image themes
  - 2 animated GIF wallpapers
  - 3 Lottie vector animations
  - 3 dynamic algorithm presets
  - 2 adaptive time-based presets
- [ ] Define ownership and throughput:
    - Engineering owner: preset runtime validation, conversion tooling, packaging
    - Design owner: source artwork, motion guidelines, preview composition
    - QA owner: visual correctness across target resolutions and themes
- [ ] Add authoring pipeline:
    - Source format: layered design files + exported master assets
    - Conversion: scripted resize/compress and metadata generation
    - Validation: schema + size/fps/heap checks in CI
- [ ] Implement `PresetLibrary` class
- [ ] Create preset JSON format and loader
- [ ] Generate preset thumbnails
- [ ] Create Preset Gallery UI in Settings
- [ ] Add "Apply Preset" button
- [ ] Add release gate: no preset is shipped without thumbnail, attribution metadata, and passing perf envelope

**Files to Create:**
- `UI/Include/flx/ui/wallpaper/PresetLibrary.hpp`
- `UI/Source/wallpaper/PresetLibrary.cpp`
- `/data/wallpapers/presets/[preset_id]/*.json`
- Applications tab: WallpaperGallery

**Deliverable**: Beautiful out-of-box wallpaper experience with preset browser

### Phase 6: Adaptive Wallpapers (Weeks 11-12)

**Goal**: Context-aware wallpapers

**Tasks:**
- [ ] Implement `AdaptiveProvider`
- [ ] Integrate with time service (morning/afternoon/evening/night)
- [ ] Integrate with weather service (sunny/cloudy/rainy/snowy)
- [ ] Integrate with battery service (charging/normal/low)
- [ ] Create adaptive wallpaper presets
- [ ] Add adaptive mode toggle to Settings

**Files to Create:**
- `UI/Source/wallpaper/providers/AdaptiveProvider.cpp`

**Deliverable**: Wallpapers automatically change based on time, weather, and system state

### Phase 7: Polish & Optimization (Weeks 13+)

**Tasks:**
- [ ] Performance profiling and optimization
- [ ] Memory leak testing
- [ ] Edge case handling (missing files, corrupted presets)
- [ ] Documentation (user guide, developer guide)
- [ ] Community wallpaper format specification
- [ ] Optional: Wallpaper preview thumbnails in settings
- [ ] Optional: Wallpaper marketplace URL integration

---

## Technical Considerations

### Memory Constraints

**ESP32-S3 Resources:**
- **RAM**: ~320KB available for user code (after kernel/OS)
- **FLASH**: ~1.5MB SPIFFS for user storage
- **PSRAM**: Up to 8MB (if available on board)

**Memory Budget:**
```
Wallpaper Image Cache:    ~200KB max
Active Wallpaper Object:  ~50KB
Preset Library:           ~50KB
Effect Pipeline:          ~20KB
Provider Instances:       ~30KB
Total Target:             ~350KB
```

**Optimization Strategies:**
1. **Image Caching**: LVGL image cache with size limits
2. **Compression**: Use WebP format (30-50% smaller than PNG)
3. **Lazy Loading**: Load presets on-demand
4. **Memory Pooling**: Reuse buffers for canvas rendering
5. **PSRAM Support**: Offload image cache to PSRAM if available

### Performance Requirements

**Target Metrics:**
- **Frame Rate**: 30-60 FPS (30 FPS minimum)
- **Frame Time Budget**: 16-33ms per frame
- **CPU Usage**: <50% average for wallpaper engine
- **Adaptive Degradation**: Reduce animation complexity on frame drops

**Performance Monitoring:**
```cpp
// Every frame
auto metric = PerformanceMonitor::measureFrame();
if (metric.fps < 25.0f) {
    // Reduce quality: lower canvas resolution, fewer particles, etc.
    WallpaperManager::getInstance().setQualityLevel(QUALITY_LOW);
}
```

### Display and Resolution

**Supported Resolutions:**
- 240×320 (common on small displays)
- 280×320 (Lilygo T-HMI)
- 320×480 (standard mobile)
- 480×800 (larger displays)

**Adaptive Rendering:**
- Canvas size reduces with large displays to manage memory
- Upscale canvas output to display resolution
- Example: 240×240 canvas scaled to 480×800

### File Format Recommendations

| Need | Format | Size | Speed | Scalability |
|------|--------|------|-------|-------------|
| **Photo** | WebP | Small | Fast | No |
| **Illustration** | PNG | Medium | Fast | No |
| **Animation (simple)** | GIF | Large | Medium | No |
| **Animation (complex)** | Lottie JSON | Very Small | Fast | Yes |
| **Vector Graphics** | SVG | Small | Fast | Yes |
| **Generative** | Algorithm | Tiny | CPU-bound | Yes |

### Thread Safety

**Update Sources:**
- Settings service thread (wallpaper change)
- File system thread (image loading)
- UI event thread (render callbacks)
- EventBus subscribers

**Execution Model (authoritative):**
- Only the LVGL/UI task may create, mutate, or destroy `lv_obj_t` objects.
- Any non-UI task (settings, filesystem, network, event bus) publishes a command to a lock-free queue.
- The UI task drains the queue once per tick and applies all UI mutations inside the LVGL lock window.

**Command Queue Contract:**
- `WallpaperCommand` payload types:
    - `SET_SOURCE(path, type)`
    - `SET_EFFECTS(json)`
    - `SET_SPEED(value)`
    - `SET_QUALITY(level)`
    - `PROVIDER_SWITCH(type)`
    - `FALLBACK_STATIC(path, reason)`
- Queue is single-consumer (UI task), multi-producer (all other tasks).
- Producers never call LVGL APIs directly.

**Memory and Data Handover Rules:**
- Producers allocate immutable command payloads.
- UI consumer owns payload free-after-apply.
- Large decode buffers are double-buffered and swapped via atomic pointer exchange.
- Provider switch uses two-phase commit:
    1. Instantiate and warm candidate provider off-screen.
    2. Atomically swap active provider in UI task.

**Locking Policy:**
- `GuiLock` protects LVGL critical sections in the UI task.
- One mutex guards provider registry and lifecycle state.
- One RW lock guards preset metadata store.
- No nested lock ordering except: `provider_mutex` -> `GuiLock` to avoid deadlocks.

**Failure Policy:**
- If command queue is full, drop oldest non-critical command (`SET_SPEED`, visual-only effects).
- Critical commands (`SET_SOURCE`, `FALLBACK_STATIC`) are retried with bounded backoff.
- Any provider error emits `wallpaper.error` and enqueues `FALLBACK_STATIC(default_path, reason)`.

---

## Storage & File Structure

### Filesystem Organization

```
flxos/
├── WALLPAPER_ENGINE_PLAN.md          # This file
├── System/
│   ├── Include/flx/core/managers/
│   │   └── WallpaperManager.hpp
│   └── Source/managers/
│       ├── WallpaperManager.cpp
│       └── WallpaperManager_test.cpp
├── UI/
│   ├── Include/flx/ui/wallpaper/
│   │   ├── IWallpaperProvider.hpp
│   │   ├── EffectPipeline.hpp
│   │   ├── PerformanceMonitor.hpp
│   │   ├── PresetLibrary.hpp
│   │   └── WallpaperWidget.hpp
│   └── Source/
│       ├── wallpaper/
│       │   ├── providers/
│       │   │   ├── StaticImageProvider.cpp
│       │   │   ├── AnimatedGifProvider.cpp
│       │   │   ├── LottieProvider.cpp
│       │   │   ├── DynamicProvider.cpp
│       │   │   └── AdaptiveProvider.cpp
│       │   ├── effects/
│       │   │   ├── BlurEffect.cpp
│       │   │   ├── BrightnessEffect.cpp
│       │   │   ├── FadeTransitionEffect.cpp
│       │   │   ├── ParallaxEffect.cpp
│       │   │   └── ColorOverlayEffect.cpp
│       │   ├── EffectPipeline.cpp
│       │   ├── PerformanceMonitor.cpp
│       │   ├── PresetLibrary.cpp
│       │   └── WallpaperWidget.cpp
│       └── desktop/
│           └── Desktop.cpp (modified for engine integration)
├── Applications/
│   ├── wallpaper_engine/
│   │   ├── WallpaperEngineApp.cpp
│   │   ├── WallpaperEngineApp.hpp
│   │   ├── pages/
│   │   └── widgets/
│   └── settings/
│       ├── wallpaper/
│       │   ├── WallpaperSettings.cpp
│       │   ├── WallpaperSettings.hpp
│       │   ├── PresetGallery.cpp
│       │   └── PresetGallery.hpp
│       └── display/
│           └── DisplaySettings.hpp (update for wallpaper type selection)
└── storage/
    └── data/
        ├── settings/
        │   ├── wp_enabled
        │   ├── wp_type
        │   ├── wp_source
        │   └── wp_effects
        └── wallpapers/
            ├── presets/
            │   ├── builtin/
            │   │   ├── sunset/
            │   │   │   ├── config.json
            │   │   │   ├── wallpaper.png
            │   │   │   └── thumbnail.png
            │   │   ├── ocean/
            │   │   ├── forest/
            │   │   └── ...
            │   └── user/
            │       ├── preset_1/
            │   │       ├── config.json
            │   │       ├── wallpaper.gif
            │   │       └── thumbnail.png
            │       └── ...
            └── cache/
                ├── thumbnail_cache/
                └── decoded_images/
```

### Preset Configuration Format

**File**: `/data/wallpapers/presets/builtin/sunset/config.json`

```json
{
  "id": "sunset",
  "name": "Beautiful Sunset",
  "description": "Warm orange and purple gradient sunset",
  "version": "1.0",
  "type": "static",
  "source": "/data/wallpapers/presets/builtin/sunset/wallpaper.png",
  "effects": {
    "brightness": 1.0,
    "blur": 0,
    "color_overlay": {
      "enabled": false,
      "color": "#000000",
      "opacity": 0.2
    }
  },
  "metadata": {
    "author": "FlxOS Team",
    "tags": ["sunset", "warm", "nature", "relaxing"],
    "thumbnail": "/data/wallpapers/presets/builtin/sunset/thumbnail.png",
    "rating": 4.5,
    "downloads": 1250
  }
}
```

### Settings Storage

**In SettingsManager (NVS or SPIFFS):**
```
Key: "wp_enabled"      → Value: "1" (0 or 1)
Key: "wp_type"         → Value: "static" | "animated" | "lottie" | "dynamic" | "adaptive"
Key: "wp_source"       → Value: "/data/wallpapers/presets/sunset/wallpaper.png"
Key: "wp_effects"      → Value: JSON string with effect configuration
Key: "wp_animation_speed" → Value: "50" (0-100)
Key: "wp_quality_level"   → Value: "1" (0=low, 1=normal, 2=high)
```

---

## API Reference

### WallpaperManager API

```cpp
// Get instance
WallpaperManager& manager = WallpaperManager::getInstance();

// Setting wallpapers
manager.setWallpaper("/path/to/image.png", "static");
manager.setWallpaper("/data/wallpapers/presets/ocean/wallpaper.gif", "animated");
manager.setWallpaper("algo://plasma?speed=50", "dynamic");

// Effects
manager.applyEffect("blur", Json::Value(5));           // radius: 5
manager.applyEffect("brightness", Json::Value(1.2));   // multiplier: 1.2
manager.removeEffect("blur");

// Animation control
manager.setAnimationSpeed(75);      // 0-100 percent
manager.setQualityLevel(HIGH);      // LOW / NORMAL / HIGH

// Status queries
bool animated = manager.getCurrentProvider()->isAnimated();
size_t memory = manager.getCurrentProvider()->getMemoryUsage();
```

### Observable Properties

```cpp
// Subscribe to wallpaper changes
auto subject = manager.getWallpaperSourceSubject();
lv_subject_add_observer(
    subject,
    [](lv_observer_t* observer, lv_subject_t* subject) {
        const char* path = (const char*)lv_subject_get_pointer(subject);
        ESP_LOGI("APP", "Wallpaper changed to: %s", path);
    }
);

// Subscribe to effect changes
auto effect_subject = manager.getEffectSubject();
lv_subject_add_observer(effect_subject, [](lv_observer_t* obs, lv_subject_t* subj) {
    // React to effect changes
});
```

### Preset Library API

```cpp
PresetLibrary& library = PresetLibrary::getInstance();

// List presets
auto presets = library.listPresets();
for (const auto* preset : presets) {
    ESP_LOGI("PRESETS", "%s: %s", preset->id.c_str(), preset->name.c_str());
}

// Apply preset
library.applyPreset("sunset", &manager);

// User presets
WallpaperPreset custom;
custom.id = "my_wallpaper";
custom.name = "My Custom Wallpaper";
custom.type = "static";
custom.source = "/sd/my_wallpaper.png";
library.saveUserPreset(custom);
```

---

## Hard-Part De-Risk Plan

This section converts the highest-risk implementation areas into explicit, testable spikes before full feature rollout.

### Spike 1: GIF Decode and Playback Envelope

**Objective:** Prove viable GIF playback on target boards without UI stalls.

**Approach:**
- Build a standalone decode benchmark with representative assets (small, medium, worst-case).
- Measure decode time per frame, frame pacing jitter, and heap fragmentation.
- Use bounded frame cache (N frames max) and decode-ahead worker.
- Drop to static fallback if the playback envelope is violated for 2 seconds.

**Acceptance Gates:**
- 240x320: >= 24 FPS sustained for 60 seconds.
- 320x480: >= 20 FPS sustained for 60 seconds.
- Extra heap under playback <= 180KB.
- No watchdog resets, no UI lockups.

### Spike 2: Lottie/ThorVG Viability on ESP32

**Objective:** Validate practical Lottie complexity limits on real hardware.

**Approach:**
- Define scene complexity tiers (low/medium/high) by shape count and layer count.
- Profile render frame time and peak memory for each tier.
- Add runtime complexity scoring from preset metadata.
- Reject or downgrade scenes that exceed budget.

**Acceptance Gates:**
- Medium complexity scenes render at >= 24 FPS.
- Peak additional heap <= 220KB.
- Fallback to static poster frame on parse/render error.

### Spike 3: Hot Provider Switching

**Objective:** Eliminate flicker and race conditions during provider transitions.

**Approach:**
- Warm new provider off-screen.
- Atomic active-provider swap in UI task.
- Delay old provider destruction until one successful frame is presented.

**Acceptance Gates:**
- No black frame during switch.
- No leaked `lv_obj_t` after repeated switch loops (1000 iterations).

---

## LVGL and FreeRTOS Concurrency Model

### Non-Negotiable Rules

1. LVGL API calls are allowed only on the UI task.
2. Non-UI tasks communicate via command queue only.
3. Provider lifecycle changes happen on UI task.

### Task Topology

- `ui_task`: runs `lv_timer_handler()`, drains wallpaper command queue, applies UI mutations.
- `io_task`: filesystem reads, decode preparation, metadata parsing.
- `settings_task`: persistence and setting changes.
- `event_task`: EventBus fan-out.

### Queue and Data Flow

- `settings_task` posts `SET_SOURCE` to queue.
- `io_task` validates file and optional decode preflight, posts `SOURCE_READY` or `SOURCE_FAILED`.
- `ui_task` applies source, starts provider, emits success/failure events.

### Reference State Machine

- `IDLE` -> `LOADING` -> `ACTIVE`
- `LOADING` -> `FAILED` -> `FALLBACK_ACTIVE`
- `ACTIVE` -> `SWITCHING` -> `ACTIVE`

### Deadlock and Stall Prevention

- Max queue wait in producers: 10ms, then backoff.
- UI apply budget: <= 4ms per tick for wallpaper commands.
- If budget exceeded for 30 consecutive ticks, auto-raise `QUALITY_LOW`.

---

## Preset Production Pipeline

The preset goal is feasible only with explicit ownership and automation.

### Roles and Ownership

- Design owner: produces source art/motion assets and visual specs.
- Engineering owner: conversion scripts, runtime validation, packaging.
- QA owner: cross-device verification and regression checks.

### Authoring to Shipping Flow

1. Design exports master asset and poster thumbnail.
2. Conversion script generates target variants and compressed outputs.
3. Metadata generator emits `config.json` with complexity and budget hints.
4. CI runs schema checks, size limits, and perf envelope tests.
5. Approved preset is signed off and added to builtin catalog.

### Definition of Done for Each Preset

- Has attribution metadata and license tag.
- Includes thumbnail and poster fallback.
- Passes size envelope:
    - static <= 350KB
    - animated gif <= 1.2MB
    - lottie <= 250KB
- Passes perf envelope on at least one ESP32 and one ESP32-S3 profile.

### Capacity Planning

- Target throughput: 3 ship-ready presets per week.
- Phase 5 target (10-15 presets) requires 4-5 weeks of parallel content production and engineering validation.

---

## Risk Assessment

### High-Risk Areas

| Risk | Severity | Mitigation |
|------|----------|-----------|
| Memory exhaustion from large wallpapers | 🔴 HIGH | Image cache limits, WebP compression, PSRAM support |
| CPU spiking crashes system | 🔴 HIGH | PerformanceMonitor + quality adaptation, frame rate limiting |
| FileSystem access blocks UI | 🟡 MEDIUM | Async file loading, thread-safe operations |
| Complex animations cause jank | 🟡 MEDIUM | Adaptive quality, frame rate caps, profiling tools |
| Preset corrupted causes crash | 🟡 MEDIUM | JSON validation, error handling, fallback to default |

### Testing Requirements

**Unit Tests:**
- Provider interface implementations
- Effect pipeline composition
- Preset JSON loading
- Observable property updates

**Integration Tests:**
- Wallpaper switching between types
- Effect application and removal
- Memory monitoring
- File loading on SD card
- Settings persistence and restore

**Performance Tests:**
- FPS monitoring with different wallpapers
- Memory usage profiling
- CPU usage measurement
- Thermal impact

---

## Success Metrics

### Phase-by-Phase Success Criteria

| Phase | Success Criteria |
|-------|------------------|
| **Phase 1** | Existing wallpaper functionality works through new engine |
| **Phase 2** | GIF wallpapers animate smoothly, Lottie renders correctly |
| **Phase 3** | Effects apply without visual artifacts, smooth transitions |
| **Phase 4** | Dynamic wallpapers render at 30+ FPS, quality adapts to load |
| **Phase 5** | Users can browse and apply presets from UI gallery |
| **Phase 6** | Wallpapers change automatically based on time/weather |
| **Phase 7** | Memory stable, no crashes, user documentation complete |

### Final Deliverables

✅ Wallpaper Engine Service  
✅ 5 Wallpaper Provider Types  
✅ Effect Pipeline System  
✅ 15+ Beautiful Presets  
✅ Performance Monitoring  
✅ Settings UI Integration  
✅ Documentation & Examples  
✅ Test Suite  

---

## Conclusion

This wallpaper engine will elevate FlxOS from a basic embedded OS to a polished desktop experience rivaling commercial OS. The modular provider architecture enables extensibility, while LVGL's rich drawing capabilities provide impressive visual effects on resource-constrained hardware.

**Next Steps:**
1. Review and approve this plan
2. Create development branch
3. Begin Phase 1 implementation
4. Iterate through phases with community feedback

---

**Document Version**: 1.1  
**Last Updated**: March 25, 2026  
**Status**: Ready for Implementation  
**Maintainer**: FlxOS Development Team
