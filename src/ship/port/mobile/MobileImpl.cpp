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

extern "C" void JNICALL Java_com_ghostship_android_MainActivity_attachController(JNIEnv* env, jobject obj) {
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

extern "C" void JNICALL Java_com_ghostship_android_MainActivity_setCameraState(JNIEnv* env, jobject jobj, jint axis,
                                                                              jfloat value) {
    switch (axis) {
        case 0:
            cameraYaw = value;
            break;
        case 1:
            cameraPitch = value;
            break;
    }
}

extern "C" void JNICALL Java_com_ghostship_android_MainActivity_setButton(JNIEnv* env, jobject jobj, jint button,
                                                                          jboolean value) {
    if (button < 0) {
        SDL_JoystickSetVirtualAxis(virtual_joystick, -button,
                                   value ? SDL_MAX_SINT16 : -SDL_MAX_SINT16); // SDL virtual axis bug workaround
    } else {
        SDL_JoystickSetVirtualButton(virtual_joystick, button, value);
    }
}

extern "C" void JNICALL Java_com_ghostship_android_MainActivity_setAxis(JNIEnv* env, jobject jobj, jint axis,
                                                                        jshort value) {
    SDL_JoystickSetVirtualAxis(virtual_joystick, axis, value);
}

extern "C" void JNICALL Java_com_ghostship_android_MainActivity_detachController(JNIEnv* env, jobject jobj) {
    SDL_JoystickClose(virtual_joystick);
    SDL_JoystickDetachVirtual(virtual_joystick_id);
    virtual_joystick = nullptr;
    virtual_joystick_id = -1;
    isUsingTouchscreenControls = false;
}

extern "C" JNIEXPORT void JNICALL
Java_com_ghostship_android_MainActivity_nativeHandleSelectedFile(JNIEnv* env, jobject thiz, jstring filename) {
}

#endif
