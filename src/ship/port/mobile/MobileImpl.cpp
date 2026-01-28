#if defined(__ANDROID__) || defined(__IOS__)
#include "ship/port/mobile/MobileImpl.h"
#include <SDL2/SDL.h>
#include "libultraship/bridge/consolevariablebridge.h"

#include <imgui_internal.h>

static float cameraYaw;
static float cameraPitch;

static bool isShowingVirtualKeyboard = true;
static bool isUsingTouchscreenControls = true;

void Ship::Mobile::ImGuiProcessEvent(bool wantsTextInput) {
    ImGuiInputTextState* state = ImGui::GetInputTextState(ImGui::GetActiveID());

    if (wantsTextInput) {
        if (!isShowingVirtualKeyboard) {
            state->ClearText();

            isShowingVirtualKeyboard = true;
            SDL_StartTextInput();
        }
    } else {
        if (isShowingVirtualKeyboard) {
            isShowingVirtualKeyboard = false;
            SDL_StopTextInput();
        }
    }
}

#endif

#ifdef __ANDROID__
#include <SDL2/SDL_gamecontroller.h>
#include <jni.h>

#ifndef SHIP_ANDROID_JNI_CLASS
// JNI-mangled class name (dots replaced with underscores). Example: com_example_android_MainActivity
#define SHIP_ANDROID_JNI_CLASS com_libultraship_android_MainActivity
#endif

#define SHIP_ANDROID_JNI_METHOD(name) Java_##SHIP_ANDROID_JNI_CLASS##_##name

bool Ship::Mobile::IsUsingTouchscreenControls() {
    return isUsingTouchscreenControls;
}

void Ship::Mobile::EnableTouchArea() {
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    jobject javaObject = (jobject)SDL_AndroidGetActivity();
    jclass javaClass = env->GetObjectClass(javaObject);
    jmethodID enabletoucharea = env->GetMethodID(javaClass, "EnableTouchArea", "()V");
    env->CallVoidMethod(javaObject, enabletoucharea);
}

void Ship::Mobile::DisableTouchArea() {
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    jobject javaObject = (jobject)SDL_AndroidGetActivity();
    jclass javaClass = env->GetObjectClass(javaObject);
    jmethodID disabletoucharea = env->GetMethodID(javaClass, "DisableTouchArea", "()V");
    env->CallVoidMethod(javaObject, disabletoucharea);
}

void Ship::Mobile::SetMenuOpen(bool open) {
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    jobject javaObject = (jobject)SDL_AndroidGetActivity();
    jclass javaClass = env->GetObjectClass(javaObject);
    jmethodID setMenuOpen = env->GetMethodID(javaClass, "setMenuOpen", "(Z)V");
    if (setMenuOpen == nullptr) {
        return;
    }
    env->CallVoidMethod(javaObject, setMenuOpen, (jboolean)open);
}

float Ship::Mobile::GetCameraYaw() {
    return cameraYaw;
}

float Ship::Mobile::GetCameraPitch() {
    return cameraPitch;
}

static int virtual_joystick_id = -1;
static SDL_Joystick* virtual_joystick = nullptr;

static void AttachControllerNative() {
    virtual_joystick_id = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 18, 0);
    if (virtual_joystick_id == -1) {
        SDL_Log("Could not create overlay virtual controller");
        return;
    }

    virtual_joystick = SDL_JoystickOpen(virtual_joystick_id);
    if (virtual_joystick == nullptr) {
        SDL_Log("Could not create virtual joystick");
    }

    isUsingTouchscreenControls = true;
}

static void SetCameraStateNative(jint axis, jfloat value) {
    switch (axis) {
        case 0:
            cameraYaw = value;
            break;
        case 1:
            cameraPitch = value;
            break;
    }
}

static void SetButtonNative(jint button, jboolean value) {
    if (button < 0) {
        SDL_JoystickSetVirtualAxis(virtual_joystick, -button,
                                   value ? SDL_MAX_SINT16 : -SDL_MAX_SINT16); // SDL virtual axis bug workaround
    } else {
        SDL_JoystickSetVirtualButton(virtual_joystick, button, value);
    }
}

static void SetAxisNative(jint axis, jshort value) {
    SDL_JoystickSetVirtualAxis(virtual_joystick, axis, value);
}

static void DetachControllerNative() {
    SDL_JoystickClose(virtual_joystick);
    SDL_JoystickDetachVirtual(virtual_joystick_id);
    virtual_joystick = nullptr;
    virtual_joystick_id = -1;
    isUsingTouchscreenControls = false;
}

static void HandleSelectedFileNative(JNIEnv* env, jstring filename) {
    (void)env;
    (void)filename;
}

extern "C" JNIEXPORT void JNICALL SHIP_ANDROID_JNI_METHOD(attachController)(JNIEnv* env, jobject obj) {
    (void)env;
    (void)obj;
    AttachControllerNative();
}

extern "C" JNIEXPORT void JNICALL SHIP_ANDROID_JNI_METHOD(setCameraState)(JNIEnv* env, jobject jobj, jint axis,
                                                                          jfloat value) {
    (void)env;
    (void)jobj;
    SetCameraStateNative(axis, value);
}

extern "C" JNIEXPORT void JNICALL SHIP_ANDROID_JNI_METHOD(setButton)(JNIEnv* env, jobject jobj, jint button,
                                                                     jboolean value) {
    (void)env;
    (void)jobj;
    SetButtonNative(button, value);
}

extern "C" JNIEXPORT void JNICALL SHIP_ANDROID_JNI_METHOD(setAxis)(JNIEnv* env, jobject jobj, jint axis,
                                                                   jshort value) {
    (void)env;
    (void)jobj;
    SetAxisNative(axis, value);
}

extern "C" JNIEXPORT void JNICALL SHIP_ANDROID_JNI_METHOD(detachController)(JNIEnv* env, jobject jobj) {
    (void)env;
    (void)jobj;
    DetachControllerNative();
}

extern "C" JNIEXPORT void JNICALL SHIP_ANDROID_JNI_METHOD(nativeHandleSelectedFile)(JNIEnv* env, jobject thiz,
                                                                                    jstring filename) {
    (void)thiz;
    HandleSelectedFileNative(env, filename);
}

#endif
