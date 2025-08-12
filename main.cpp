// #include <GLFW/glfw3.h>
// #include "include/core/SkSurface.h"
// #include "include/core/SkCanvas.h"
// #include "include/core/SkPaint.h"
// #include "include/core/SkColor.h"
// #include "include/core/SkImageInfo.h"
//
// int main() {
//     // 1. 初始化 GLFW
//     if (!glfwInit()) {
//         return -1;
//     }
//
//     // 2. 创建窗口（注意：GLFW 默认使用 OpenGL，但我们将用 Skia 的 CPU 渲染）
//     GLFWwindow* window = glfwCreateWindow(800, 600, "Skia + GLFW (CPU)", nullptr, nullptr);
//     if (!window) {
//         glfwTerminate();
//         return -1;
//     }
//
//     // 3. 创建 Skia 的 CPU 位图表面（Raster 模式）
//     const int width = 800;
//     const int height = 600;
//     sk_sp<SkSurface> surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(64, 64));
//
//     // 4. 主渲染循环
//     while (!glfwWindowShouldClose(window)) {
//         glfwPollEvents();
//         glClear(GL_RED);
//         glfwSwapBuffers(window);
//
//         continue;
//         // 获取画布并绘制
//         SkCanvas* canvas = surface->getCanvas();
//         canvas->clear(SK_ColorWHITE);  // 清空画布（白色背景）
//
//         // 绘制一条红线
//         SkPaint paint;
//         paint.setColor(SK_ColorRED);
//         paint.setStrokeWidth(3.0f);
//         paint.setStyle(SkPaint::kStroke_Style);
//         canvas->drawLine(100, 100, 700, 500, paint);
//
//         // 5. 将 Skia 位图复制到 GLFW 窗口（通过 OpenGL 纹理）
//         // 注意：这里需要手动将 CPU 数据上传到 GPU
//         SkPixmap pixmap;
//         surface->peekPixels(&pixmap);
//
//         // 创建临时 OpenGL 纹理并上传数据
//         GLuint texture;
//         glGenTextures(1, &texture);
//         glBindTexture(GL_TEXTURE_2D, texture);
//         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
//                      GL_RGBA, GL_UNSIGNED_BYTE, pixmap.addr());
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//
//         // 渲染纹理到窗口
//         glClear(GL_COLOR_BUFFER_BIT);
//         glEnable(GL_TEXTURE_2D);
//         glBegin(GL_QUADS);
//         glTexCoord2f(0, 1); glVertex2f(-1, -1);
//         glTexCoord2f(1, 1); glVertex2f(1, -1);
//         glTexCoord2f(1, 0); glVertex2f(1, 1);
//         glTexCoord2f(0, 0); glVertex2f(-1, 1);
//         glEnd();
//
//         glfwSwapBuffers(window);
//         glDeleteTextures(1, &texture);
//     }
//
//     // 6. 清理资源
//     glfwTerminate();
//     return 0;
// }
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "include/gpu/GrBackendSurface.h"
#include "include/gpu/GrDirectContext.h"
#include "include/gpu/gl/GrGLInterface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void processInput(GLFWwindow *window);

void error_callback(int error, const char *description) {
    fputs(description, stderr);
}

GrDirectContext *grDirectContext = nullptr;
SkSurface *skSurface = nullptr;

void init_Skia(int width, int height) {
    auto interface = GrGLMakeNativeInterface();
    grDirectContext = GrDirectContext::MakeGL(interface).release();

    GrGLFramebufferInfo framebufferInfo;
    framebufferInfo.fFBOID = 0;
    framebufferInfo.fFormat = GL_RGBA8;

    SkColorType colorType = kRGBA_8888_SkColorType;
    GrBackendRenderTarget backendRenderTarget(width, height, 0, 0, framebufferInfo);
    skSurface = SkSurface::MakeFromBackendRenderTarget(grDirectContext, backendRenderTarget,
                                                       kBottomLeft_GrSurfaceOrigin, colorType,
                                                       nullptr, nullptr).release();
    if (skSurface == nullptr) abort();

}

void clearnup_skia() {
    delete skSurface;
    delete grDirectContext;
}

// settings
const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

int main() {
    GLFWwindow *window;
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);

    init_Skia(WIDTH, HEIGHT);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    SkCanvas *canvas = skSurface->getCanvas();

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        SkPaint paint;
        paint.setColor(SK_ColorWHITE);
        canvas->drawPaint(paint);
        paint.setColor(SK_ColorBLUE);
        canvas->drawRect({100, 200, 300, 500}, paint);
        canvas->flush();
        grDirectContext->flushAndSubmit();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    clearnup_skia();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}


