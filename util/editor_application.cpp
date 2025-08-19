// Copyright 2019 Google LLC.
// Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.

// Proof of principle of a text editor written with Skia & SkShaper.
// https://bugs.skia.org/9020

#include "include/core/SkCanvas.h"
#include "include/core/SkData.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkSurface.h"
#include "src/base/SkTime.h"

#include "Application.h"
#include "Windows.h"


#if defined(SK_FONTMGR_FONTCONFIG_AVAILABLE) && defined(SK_TYPEFACE_FACTORY_FREETYPE)
#include "include/ports/SkFontMgr_fontconfig.h"
#include "include/ports/SkFontScanner_FreeType.h"
#endif

#if defined(SK_FONTMGR_CORETEXT_AVAILABLE)
#include "include/ports/SkFontMgr_mac_ct.h"
#endif

#if defined(SK_FONTMGR_DIRECTWRITE_AVAILABLE)
#include "include/ports/SkTypeface_win.h"
#endif

#include <cfloat>
#include <fstream>
#include <memory>


#ifdef SK_EDITOR_DEBUG_OUT
static const char* key_name(skui::Key k) {
    switch (k) {
        #define M(X) case skui::Key::k ## X: return #X
        M(NONE); M(LeftSoftKey); M(RightSoftKey); M(Home); M(Back); M(Send); M(End); M(0); M(1);
        M(2); M(3); M(4); M(5); M(6); M(7); M(8); M(9); M(Star); M(Hash); M(Up); M(Down); M(Left);
        M(Right); M(Tab); M(PageUp); M(PageDown); M(Delete); M(Escape); M(Shift); M(Ctrl);
        M(Option); M(A); M(C); M(V); M(X); M(Y); M(Z); M(OK); M(VolUp); M(VolDown); M(Power);
        M(Camera);
        #undef M
        default: return "?";
    }
}

static SkString modifiers_desc(skui::ModifierKey m) {
    SkString s;
    #define M(X) if (m & skui::ModifierKey::k ## X ##) { s.append(" {" #X "}"); }
    M(Shift) M(Control) M(Option) M(Command) M(FirstPress)
    #undef M
    return s;
}

static void debug_on_char(SkUnichar c, skui::ModifierKey modifiers) {
    SkString m = modifiers_desc(modifiers);
    if ((unsigned)c < 0x100) {
        SkDebugf("char: %c (0x%02X)%s\n", (char)(c & 0xFF), (unsigned)c, m.c_str());
    } else {
        SkDebugf("char: 0x%08X%s\n", (unsigned)c, m.c_str());
    }
}

static void debug_on_key(skui::Key key, skui::InputState, skui::ModifierKey modi) {
    SkDebugf("key: %s%s\n", key_name(key), modifiers_desc(modi).c_str());
}
#endif  // SK_EDITOR_DEBUG_OUT


namespace {

struct Timer {
    double fTime;
    const char* fDesc;
    Timer(const char* desc = "") : fTime(SkTime::GetNSecs()), fDesc(desc) {}
    ~Timer() { SkDebugf("%s: %5d μs\n", fDesc, (int)((SkTime::GetNSecs() - fTime) * 1e-3)); }
};

static constexpr float kFontSize = 18;
static const char* kTypefaces[3] = {"sans-serif", "serif", "monospace"};
static constexpr size_t kTypefaceCount = std::size(kTypefaces);


// Note: initialization is not thread safe
sk_sp<SkFontMgr> fontMgr() {
    static bool init = false;
    static sk_sp<SkFontMgr> fontMgr = nullptr;
    if (!init) {
#if defined(SK_FONTMGR_FONTCONFIG_AVAILABLE) && defined(SK_TYPEFACE_FACTORY_FREETYPE)
        fontMgr = SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
#elif defined(SK_FONTMGR_CORETEXT_AVAILABLE)
        fontMgr = SkFontMgr_New_CoreText(nullptr);
#elif defined(SK_FONTMGR_DIRECTWRITE_AVAILABLE)
        fontMgr = SkFontMgr_New_DirectWrite();
#endif
        init = true;
    }
    return fontMgr;
}


#ifdef SK_VULKAN
static constexpr sk_app::Window::BackendType kBackendType = sk_app::Window::kVulkan_BackendType;
#elif SK_METAL
static constexpr sk_app::Window::BackendType kBackendType = sk_app::Window::kMetal_BackendType;
#elif SK_GL
static constexpr sk_app::Window::BackendType kBackendType = sk_app::Window::kNativeGL_BackendType;
#else
static constexpr sk_app::Window::BackendType kBackendType = sk_app::Window::kRaster_BackendType;
#endif

struct EditorLayer : public sk_app::Window::Layer {
    void onPaint(SkSurface* surface) override {
        SkCanvas* canvas = surface->getCanvas();
        // 清空画布（白色背景）
        canvas->clear(SK_ColorWHITE);

        // 绘制一条红线
        SkPaint paint;
        paint.setColor(SK_ColorRED);
        paint.setStrokeWidth(3.0f);
        paint.setStyle(SkPaint::kStroke_Style);
        canvas->drawLine(100, 100, 100, 500, paint);
    }
};

struct EditorApplication : public sk_app::Application {
    std::unique_ptr<sk_app::Window> fWindow;
    double fNextTime = -DBL_MAX;
    EditorLayer fLayer;

    EditorApplication(std::unique_ptr<sk_app::Window> win) : fWindow(std::move(win)) {}

    bool init(const char* path) {
        fWindow->attach(kBackendType);

        fWindow->setTitle("skia");
        fWindow->pushLayer(&fLayer);

        fWindow->show();
        return true;
    }
    ~EditorApplication() override { fWindow->detach(); }

    void onIdle() override {
        double now = SkTime::GetNSecs();
        if (now >= fNextTime) {
            constexpr double kHalfPeriodNanoSeconds = 0.5 * 1e9;
            fNextTime = now + kHalfPeriodNanoSeconds;
            fWindow->inval();
        }

    }

};
}  // namespace

sk_app::Application* sk_app::Application::Create(int argc, char** argv, void* dat) {
    std::unique_ptr<sk_app::Window> win(sk_app::Windows::CreateNativeWindow(dat));
    if (!win) {
        SK_ABORT("CreateNativeWindow failed.");
    }
    std::unique_ptr<EditorApplication> app(new EditorApplication(std::move(win)));
    (void)app->init(argc > 1 ? argv[1] : nullptr);
    return app.release();
}
