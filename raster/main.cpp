#include <SDL.h>
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkColor.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPixmap.h"

int main() {
    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    const int width = 800;
    const int height = 600;

    // 创建SDL窗口（使用软件渲染）
    SDL_Window* window = SDL_CreateWindow(
        "Skia + SDL2 (CPU)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // 获取SDL表面（确保像素格式匹配）
    SDL_Surface* screen = SDL_GetWindowSurface(window);
    if (!screen) {
        SDL_Log("SDL_GetWindowSurface failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 创建Skia表面（尺寸与SDL窗口一致）
    sk_sp<SkSurface> surface = SkSurfaces::Raster(
        SkImageInfo::MakeN32Premul(width, height)
    );
    SkCanvas* canvas = surface->getCanvas();

    // 主循环
    bool quit = false;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        // 清空画布（白色背景）
        canvas->clear(SK_ColorWHITE);

        // 绘制一条红线
        SkPaint paint;
        paint.setColor(SK_ColorRED);
        paint.setStrokeWidth(3.0f);
        paint.setStyle(SkPaint::kStroke_Style);
        canvas->drawLine(100, 100, 100, 500, paint);

        // 将Skia像素数据拷贝到SDL表面
        SkPixmap pixmap;
        if (surface->peekPixels(&pixmap)) {
            //确保SDL表面和Skia表面的像素格式匹配
            if (screen->format->BytesPerPixel == 4) {  // 32位色深
                uint32_t* dst = static_cast<uint32_t*>(screen->pixels);
                const uint32_t* src = static_cast<const uint32_t*>(pixmap.addr());
                for (int y = 0; y < height; ++y) {
                    memcpy(dst + y * screen->pitch / 4,
                           src + y * pixmap.rowBytes() / 4,
                           width * 4);
                }
            }
        }

        // 更新窗口
        SDL_UpdateWindowSurface(window);
    }

    // 清理资源
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}