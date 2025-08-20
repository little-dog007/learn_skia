/*
* Copyright 2019 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#include <Carbon/Carbon.h>

#include "include/core/SkTypes.h"
#include "Window_mac.h"
#include "DisplayParams.h"
#include "WindowContext.h"

#if defined(SK_GANESH) && defined(SK_ANGLE)
#include "tools/window/mac/GaneshANGLEWindowContext_mac.h"
#endif

#if defined(USE_METAL)
#include "GaneshMetalWindowContext_mac.h"
#else
#include "GaneshGLWindowContext_mac.h"
#endif

#if defined(SK_GRAPHITE) && defined(SK_METAL)
#include "tools/window/mac/GraphiteNativeMetalWindowContext_mac.h"
#endif

#if defined(SK_GRAPHITE) && defined(SK_DAWN)
#include "tools/window/mac/GraphiteDawnMetalWindowContext_mac.h"
#endif

#include "MacWindowInfo.h"

@interface WindowDelegate : NSObject<NSWindowDelegate>

- (WindowDelegate*)initWithWindow:(sk_app::Window_mac*)initWindow;

@end

@interface MainView : NSView

- (MainView*)initWithWindow:(sk_app::Window_mac*)initWindow;

@end

///////////////////////////////////////////////////////////////////////////////

using sk_app::Window;
using skwindow::DisplayParams;

namespace sk_app {

SkTDynamicHash<Window_mac, NSInteger> Window_mac::gWindowMap;

Window* Windows::CreateNativeWindow(void*) {
    Window_mac* window = new Window_mac();
    if (!window->initWindow()) {
        delete window;
        return nullptr;
    }

    return window;
}

bool Window_mac::initWindow() {
    // we already have a window
    if (fWindow) {
        return true;
    }

    // Create a delegate to track certain events
    WindowDelegate* delegate = [[WindowDelegate alloc] initWithWindow:this];
    if (nil == delegate) {
        return false;
    }

    // Create Cocoa window
    constexpr int initialWidth = 1280;
    constexpr int initialHeight = 960;
    NSRect windowRect = NSMakeRect(100, 100, initialWidth, initialHeight);

    NSUInteger windowStyle = (NSTitledWindowMask | NSClosableWindowMask | NSResizableWindowMask |
                              NSMiniaturizableWindowMask);

    fWindow = [[NSWindow alloc] initWithContentRect:windowRect styleMask:windowStyle
                                backing:NSBackingStoreBuffered defer:NO];
    if (nil == fWindow) {
        [delegate release];
        return false;
    }

    // create view
    MainView* view = [[MainView alloc] initWithWindow:this];
    if (nil == view) {
        [fWindow release];
        [delegate release];
        return false;
    }

    [fWindow setContentView:view];
    [fWindow makeFirstResponder:view];
    [fWindow setDelegate:delegate];
    [fWindow setAcceptsMouseMovedEvents:YES];
    [fWindow setRestorable:NO];

    // Should be retained by window now
    [view release];

    fWindowNumber = fWindow.windowNumber;
    gWindowMap.add(this);

    return true;
}

void Window_mac::closeWindow() {
    if (nil != fWindow) {
        gWindowMap.remove(fWindowNumber);
        if (sk_app::Window_mac::gWindowMap.count() < 1) {
            [NSApp terminate:fWindow];
        }
        [fWindow close];
        fWindow = nil;
    }
}

void Window_mac::setTitle(const char* title) {
    if (NSString* titleStr = [NSString stringWithUTF8String:title]) {
        [fWindow setTitle:titleStr];
    }
}

void Window_mac::show() {
    [fWindow orderFront:nil];

    [NSApp activateIgnoringOtherApps:YES];
    [fWindow makeKeyAndOrderFront:NSApp];
}

bool Window_mac::attach(BackendType attachType) {
    this->initWindow();

    skwindow::MacWindowInfo info;
    info.fMainView = [fWindow contentView];

#if defined(USE_METAL)
        fWindowContext = MakeGaneshMetalForMac(info, fRequestedDisplayParams->clone());
 #else
        fWindowContext = MakeGaneshGLForMac(info, fRequestedDisplayParams->clone());
#endif



    this->onBackendCreated();

    return SkToBool(fWindowContext);
}

float Window_mac::scaleFactor() const {
    return skwindow::GetBackingScaleFactor(fWindow.contentView);
}

void Window_mac::PaintWindows() {
    gWindowMap.foreach([&](Window_mac* window) {
        if (window->fIsContentInvalidated) {
            window->onPaint();
        }
    });
}

}   // namespace sk_app

///////////////////////////////////////////////////////////////////////////////

@implementation WindowDelegate {
    sk_app::Window_mac* fWindow;
}

- (WindowDelegate*)initWithWindow:(sk_app::Window_mac *)initWindow {
    fWindow = initWindow;

    return self;
}

- (void)windowDidResize:(NSNotification *)notification {
    NSView* view = fWindow->window().contentView;
    CGFloat scale = skwindow::GetBackingScaleFactor(view);
    fWindow->onResize(view.bounds.size.width * scale, view.bounds.size.height * scale);
    fWindow->inval();
}

- (void)windowDidChangeScreen:(NSNotification *)notification {
    NSView* view = fWindow->window().contentView;
    CGFloat scale = skwindow::GetBackingScaleFactor(view);
    fWindow->onResize(view.bounds.size.width * scale, view.bounds.size.height * scale);
    fWindow->inval();
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
    fWindow->closeWindow();

    return FALSE;
}

@end





@implementation MainView {
    sk_app::Window_mac* fWindow;
    // A TrackingArea prevents us from capturing events outside the view
    NSTrackingArea* fTrackingArea;
    // We keep track of the state of the modifier keys on each event in order to synthesize
    // key-up/down events for each modifier.
}

- (MainView*)initWithWindow:(sk_app::Window_mac *)initWindow {
    self = [super init];

    fWindow = initWindow;
    fTrackingArea = nil;

    [self updateTrackingAreas];

    return self;
}

- (void)dealloc
{
    [fTrackingArea release];
    [super dealloc];
}

- (BOOL)isOpaque {
    return YES;
}

- (BOOL)canBecomeKeyView {
    return YES;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)updateTrackingAreas {
    if (fTrackingArea != nil) {
        [self removeTrackingArea:fTrackingArea];
        [fTrackingArea release];
    }

    const NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited |
                                          NSTrackingActiveInKeyWindow |
                                          NSTrackingEnabledDuringMouseDrag |
                                          NSTrackingCursorUpdate |
                                          NSTrackingInVisibleRect |
                                          NSTrackingAssumeInside;

    fTrackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                 options:options
                                                   owner:self
                                                userInfo:nil];

    [self addTrackingArea:fTrackingArea];
    [super updateTrackingAreas];
}


- (BOOL)performKeyEquivalent:(NSEvent *)event {
    [self updateModifierKeys:event];

    // By default, unhandled key equivalents send -keyDown events; unfortunately, they do not send
    // a matching -keyUp. In other words, we can claim that we didn't handle the event and OS X will
    // turn this event into a -keyDown automatically, but we need to synthesize a matching -keyUp on
    // a later frame. Since we only read the modifiers and key code from the event, we can reuse
    // this "key-equivalent" event as a "key up".
    [self performSelector:@selector(keyUp:) withObject:event afterDelay:0.1];
    return NO;
}





-(void)flagsChanged:(NSEvent *)event {
    [self updateModifierKeys:event];
}





- (void)mouseDragged:(NSEvent *)event {
    [self updateModifierKeys:event];
    [self mouseMoved:event];
}

- (void)drawRect:(NSRect)rect {
    fWindow->onPaint();
}

@end
