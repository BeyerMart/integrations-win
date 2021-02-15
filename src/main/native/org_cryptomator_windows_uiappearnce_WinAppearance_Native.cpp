//
//  Created by Martin Beyer on 01.11.2020.
//  Copyright © 2020 Cryptomator. All rights reserved.
//

#include "org_cryptomator_windows_uiappearance_WinAppearance_Native.h"
#include <jni.h>
#include <windows.h>
#include <winreg.h>

using namespace std;

class Observer {
private:
    JavaVM *vm;
    jobject listener;
public:
    HWND window{};
    Observer(JavaVM *vm, jobject listener) : vm(vm), listener(listener) {};
    int calljvm();
    int registerWndProc();
    jobject getListener() { return listener; };
    static LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

static Observer *observer;

LRESULT CALLBACK Observer::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WCHAR *plParam = (LPWSTR) lParam;
    if (msg == WM_SETTINGCHANGE && plParam) {
        //Only call the notify methode if the correct setting changed
        if (lstrcmp(reinterpret_cast<LPCSTR>(plParam), TEXT("ImmersiveColorSet")) == 0 && observer != nullptr) {
            observer->calljvm();
        }
    } else {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return EXIT_SUCCESS;
}

int Observer::calljvm() { //notify Java
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_10) != JNI_OK) {
        return EXIT_FAILURE;
    }
    jclass listenerClass = env->GetObjectClass(listener);
    if (listenerClass == nullptr) {
        return EXIT_FAILURE;
    }
    jmethodID listenerMethodID = env->GetMethodID(listenerClass, "systemAppearanceChanged", "()V");
    if (listenerMethodID == nullptr) {
        return EXIT_FAILURE;
    }
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    env->CallVoidMethod(listener, listenerMethodID);
    return EXIT_SUCCESS;
}

//Create a hidden window to get Windows messages and with each message call WndProc
int Observer::registerWndProc() {
    HINSTANCE h2 = GetModuleHandle(nullptr);
    static char szAppName[] = "WindowsSettingsThemeListener"; //TODO get a proper name
    WNDCLASSEX wndclass;
    wndclass.cbSize = sizeof(wndclass);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = h2;
    wndclass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wndclass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndclass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_MENU);
    wndclass.lpszClassName = szAppName;
    wndclass.lpszMenuName = nullptr;

    /* Register a new window class with Windows */
    if (!RegisterClassEx(&wndclass)) {
        return EXIT_FAILURE;
    }

    /* Create a window based on our new class */

    this->window = CreateWindow(szAppName, "WindowsSettingsThemeListener",
                                WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                nullptr, nullptr, h2, nullptr);
    /* CreateWindow failed? */
    if (!this->window) {
        return EXIT_FAILURE;
    }

    /* Show and update our window */
    ShowWindow(this->window, SW_HIDE);
    UpdateWindow(this->window);
    return EXIT_SUCCESS;
}


JNIEXPORT jint JNICALL
Java_org_cryptomator_windows_uiappearance_WinAppearance_00024Native_getCurrentTheme(JNIEnv *env, jobject thisObject) {
    DWORD data{};
    DWORD dataSize = sizeof(data);
    RegGetValueA(HKEY_CURRENT_USER, R"(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)",
                 "AppsUseLightTheme", RRF_RT_DWORD, nullptr, &data, &dataSize);

    if (data) {
        return 1;
    } else {
        return 0;
    }
}

JNIEXPORT jint JNICALL
Java_org_cryptomator_windows_uiappearance_WinAppearance_00024Native_observe(JNIEnv *env, jobject thisObj,
                                                                            jobject listenerObj) {
    JavaVM *vm = nullptr;
    env->GetJavaVM(&vm);
    vm->AttachCurrentThread((void **) &env, nullptr);

    jobject listener = env->NewGlobalRef(listenerObj);
    if (listener == nullptr) {
        return EXIT_FAILURE;
    }
    observer = new Observer(vm, listener);
    if (observer->registerWndProc() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    };
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) &&
           observer != nullptr) { //GetMessage waits for the next Message and stores it in msg.

        TranslateMessage(&msg); /* for certain keyboard messages */
        DispatchMessage(&msg); /* send message to WndProc */
    }
    vm->DetachCurrentThread();
    return EXIT_SUCCESS;
}
/* NOT NEEDED, AS EVERYTHING IS DONE IN OBSERVE
JNIEXPORT jint JNICALL Java_org_cryptomator_windows_uiappearance_WinAppearance_00024Native_prepareObserving(JNIEnv *env, jobject thisObj, jobject listenerObj) {
    JavaVM *vm = nullptr;
    if (env->GetJavaVM(&vm) != JNI_OK) {
        return EXIT_FAILURE;
    }
    / *
    printf("prepping, PID: %ld\n", GetCurrentProcessId());
    printf("prepping TID: %ld\n", this_thread::get_id());
    fflush(stdout);
     * /
    jobject listener = env->NewGlobalRef(listenerObj);
    if (listener == NULL) {
        return EXIT_FAILURE;
    }
    observer = new Observer(vm, listener);
    return observer->registerWndProc();
}*/

JNIEXPORT void JNICALL
Java_org_cryptomator_windows_uiappearance_WinAppearance_00024Native_stopObserving(JNIEnv *env, jobject thisObj) {
    env->DeleteGlobalRef(observer->getListener());
    DestroyWindow(observer->window);
    delete observer;
    observer = nullptr;
    // TODO: close window (and send a last message if required)
    // store hwnd as fieldW:: done
    // TODO: cleanup window
}

JNIEXPORT void JNICALL
Java_org_cryptomator_windows_uiappearance_WinAppearance_00024Native_setToLight(JNIEnv *env, jobject thisObj) {
    HKEY hkResult;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, R"(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)", 0,
                      KEY_SET_VALUE, &hkResult)) {
        printf(R"(RegOpenKeyExA(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize) failed)"); //TODO decide if really helpfull / if it appears in cryptomators log
    }
    DWORD value{1};
    RegSetValueExA(hkResult, "AppsUseLightTheme", 0, REG_DWORD, (PBYTE) &value, sizeof(DWORD));
    RegCloseKey(hkResult);
}


JNIEXPORT void JNICALL
Java_org_cryptomator_windows_uiappearance_WinAppearance_00024Native_setToDark(JNIEnv *env, jobject thisObj) {
    HKEY hkResult;
    RegOpenKeyExA(HKEY_CURRENT_USER, R"(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)", 0,
                  KEY_SET_VALUE, &hkResult);
    DWORD value{0};
    RegSetValueExA(hkResult, "AppsUseLightTheme", 0, REG_DWORD, (PBYTE) &value, sizeof(DWORD));
    RegCloseKey(hkResult);
}