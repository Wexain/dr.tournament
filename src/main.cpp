// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Client Entry Point
// ═══════════════════════════════════════════════════════════════════════════
#include "core/engine.h"

#if defined(DRT_PLATFORM_ANDROID)
// Android NativeActivity entry point — Raylib handles the mapping
// from android_main() → main() internally via its platform layer.
#endif

int main(int argc, char** argv) {
    drt::Engine engine;
    
    if (!engine.init(argc, argv)) {
        return 1;
    }
    
    engine.run();
    engine.shutdown();
    
    return 0;
}
