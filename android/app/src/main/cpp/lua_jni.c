#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

// Cache for JVM and classes/methods (Cached on JNI_OnLoad for thread safety and performance)
static JavaVM *cached_jvm = NULL;
static jclass luastate_class = NULL;
static jmethodID luastate_wrap_id = NULL;
static jclass callback_class = NULL;
static jmethodID callback_invoke_id = NULL;
static pthread_key_t thread_key;

// Pthread destructor automatically detaches background threads when they terminate
static void detach_thread_destructor(void *env) {
    if (cached_jvm) {
        (*cached_jvm)->DetachCurrentThread(cached_jvm);
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
    cached_jvm = jvm;
    JNIEnv *env = NULL;
    if ((*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    // Cache classes and method IDs using the classloader of the main thread.
    // This solves class loading issues when native background threads query classes.
    jclass ls_local = (*env)->FindClass(env, "io/github/anaruto/lua/LuaState");
    if (ls_local) {
        luastate_class = (jclass)(*env)->NewGlobalRef(env, ls_local);
        luastate_wrap_id = (*env)->GetStaticMethodID(env, luastate_class, "wrap", "(J)Lio/github/anaruto/lua/LuaState;");
    }

    jclass cb_local = (*env)->FindClass(env, "io/github/anaruto/lua/LuaCallback");
    if (cb_local) {
        callback_class = (jclass)(*env)->NewGlobalRef(env, cb_local);
        callback_invoke_id = (*env)->GetMethodID(env, callback_class, "invoke", "(Lio/github/anaruto/lua/LuaState;)I");
    }

    // Register the thread-local storage key to track attached JNI threads
    pthread_key_create(&thread_key, detach_thread_destructor);

    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved) {
    JNIEnv *env = NULL;
    if ((*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_6) == JNI_OK) {
        if (luastate_class) {
            (*env)->DeleteGlobalRef(env, luastate_class);
            luastate_class = NULL;
        }
        if (callback_class) {
            (*env)->DeleteGlobalRef(env, callback_class);
            callback_class = NULL;
        }
    }
    pthread_key_delete(thread_key);
}

JNIEnv *get_jni_env(void) {
    JNIEnv *env = NULL;
    jint res = (*cached_jvm)->GetEnv(cached_jvm, (void**)&env, JNI_VERSION_1_6);
    if (res == JNI_EDETACHED) {
        if ((*cached_jvm)->AttachCurrentThread(cached_jvm, &env, NULL) == JNI_OK) {
            // Save the JNIEnv pointer in thread-local storage to detach thread on exit
            pthread_setspecific(thread_key, env);
        } else {
            return NULL;
        }
    }
    return env;
}

// ----------------------------------------------------------------------------
// Core lifecycle methods
// ----------------------------------------------------------------------------

JNIEXPORT jlong JNICALL Java_io_github_anaruto_lua_LuaState_nativeNewState(JNIEnv *env, jclass clazz) {
    lua_State *L = luaL_newstate();
    return (jlong)L;
}

JNIEXPORT void JNICALL Java_io_github_anaruto_lua_LuaState_nativeClose(JNIEnv *env, jobject thiz, jlong ptr) {
    lua_State *L = (lua_State *)ptr;
    if (L) {
        lua_close(L);
    }
}

JNIEXPORT void JNICALL Java_io_github_anaruto_lua_LuaState_nativeOpenLibs(JNIEnv *env, jobject thiz, jlong ptr) {
    lua_State *L = (lua_State *)ptr;
    if (L) {
        luaL_openlibs(L);
    }
}

// ----------------------------------------------------------------------------
// Script execution methods
// ----------------------------------------------------------------------------

JNIEXPORT jint JNICALL Java_io_github_anaruto_lua_LuaState_nativeLoadString(JNIEnv *env, jobject thiz, jlong ptr, jstring script) {
    lua_State *L = (lua_State *)ptr;
    if (!L || !script) return -1;
    const char *code = (*env)->GetStringUTFChars(env, script, NULL);
    int res = luaL_loadstring(L, code);
    (*env)->ReleaseStringUTFChars(env, script, code);
    return res;
}

JNIEXPORT jint JNICALL Java_io_github_anaruto_lua_LuaState_nativePCall(JNIEnv *env, jobject thiz, jlong ptr, jint nargs, jint nresults, jint errfunc) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return -1;
    return lua_pcall(L, nargs, nresults, errfunc);
}

// ----------------------------------------------------------------------------
// Global variables
// ----------------------------------------------------------------------------

JNIEXPORT jint JNICALL Java_io_github_anaruto_lua_LuaState_nativeGetGlobal(JNIEnv *env, jobject thiz, jlong ptr, jstring name) {
    lua_State *L = (lua_State *)ptr;
    if (!L || !name) return -1;
    const char *str = (*env)->GetStringUTFChars(env, name, NULL);
    int res = lua_getglobal(L, str);
    (*env)->ReleaseStringUTFChars(env, name, str);
    return res;
}

JNIEXPORT void JNICALL Java_io_github_anaruto_lua_LuaState_nativeSetGlobal(JNIEnv *env, jobject thiz, jlong ptr, jstring name) {
    lua_State *L = (lua_State *)ptr;
    if (!L || !name) return;
    const char *str = (*env)->GetStringUTFChars(env, name, NULL);
    lua_setglobal(L, str);
    (*env)->ReleaseStringUTFChars(env, name, str);
}

// ----------------------------------------------------------------------------
// Push methods
// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_io_github_anaruto_lua_LuaState_nativePushNil(JNIEnv *env, jobject thiz, jlong ptr) {
    lua_State *L = (lua_State *)ptr;
    if (L) lua_pushnil(L);
}

JNIEXPORT void JNICALL Java_io_github_anaruto_lua_LuaState_nativePushBoolean(JNIEnv *env, jobject thiz, jlong ptr, jboolean val) {
    lua_State *L = (lua_State *)ptr;
    if (L) lua_pushboolean(L, val);
}

JNIEXPORT void JNICALL Java_io_github_anaruto_lua_LuaState_nativePushNumber(JNIEnv *env, jobject thiz, jlong ptr, jdouble val) {
    lua_State *L = (lua_State *)ptr;
    if (L) lua_pushnumber(L, val);
}

JNIEXPORT void JNICALL Java_io_github_anaruto_lua_LuaState_nativePushInteger(JNIEnv *env, jobject thiz, jlong ptr, jlong val) {
    lua_State *L = (lua_State *)ptr;
    if (L) lua_pushinteger(L, (lua_Integer)val);
}

JNIEXPORT void JNICALL Java_io_github_anaruto_lua_LuaState_nativePushString(JNIEnv *env, jobject thiz, jlong ptr, jstring val) {
    lua_State *L = (lua_State *)ptr;
    if (!L || !val) return;
    const char *str = (*env)->GetStringUTFChars(env, val, NULL);
    lua_pushstring(L, str);
    (*env)->ReleaseStringUTFChars(env, val, str);
}

// ----------------------------------------------------------------------------
// Read methods
// ----------------------------------------------------------------------------

JNIEXPORT jboolean JNICALL Java_io_github_anaruto_lua_LuaState_nativeToBoolean(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return JNI_FALSE;
    return lua_toboolean(L, index) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jdouble JNICALL Java_io_github_anaruto_lua_LuaState_nativeToNumber(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return 0.0;
    return lua_tonumber(L, index);
}

JNIEXPORT jlong JNICALL Java_io_github_anaruto_lua_LuaState_nativeToInteger(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return 0;
    return (jlong)lua_tointeger(L, index);
}

JNIEXPORT jstring JNICALL Java_io_github_anaruto_lua_LuaState_nativeToString(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return NULL;
    const char *str = lua_tostring(L, index);
    if (!str) return NULL;
    return (*env)->NewStringUTF(env, str);
}

JNIEXPORT jint JNICALL Java_io_github_anaruto_lua_LuaState_nativeType(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return -1;
    return lua_type(L, index);
}

JNIEXPORT jint JNICALL Java_io_github_anaruto_lua_LuaState_nativeGetTop(JNIEnv *env, jobject thiz, jlong ptr) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return 0;
    return lua_gettop(L);
}

JNIEXPORT void JNICALL Java_io_github_anaruto_lua_LuaState_nativePop(JNIEnv *env, jobject thiz, jlong ptr, jint n) {
    lua_State *L = (lua_State *)ptr;
    if (L) lua_pop(L, n);
}

// ----------------------------------------------------------------------------
// Table methods
// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_io_github_anaruto_lua_LuaState_nativeNewTable(JNIEnv *env, jobject thiz, jlong ptr) {
    lua_State *L = (lua_State *)ptr;
    if (L) lua_newtable(L);
}

JNIEXPORT jint JNICALL Java_io_github_anaruto_lua_LuaState_nativeGetTable(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return -1;
    return lua_gettable(L, index);
}

JNIEXPORT void JNICALL Java_io_github_anaruto_lua_LuaState_nativeSetTable(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (L) lua_settable(L, index);
}

JNIEXPORT jint JNICALL Java_io_github_anaruto_lua_LuaState_nativeRawGet(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return -1;
    return lua_rawget(L, index);
}

JNIEXPORT void JNICALL Java_io_github_anaruto_lua_LuaState_nativeRawSet(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (L) lua_rawset(L, index);
}

// ----------------------------------------------------------------------------
// Callback infrastructure
// ----------------------------------------------------------------------------

// Garbage collection callback for Java callback objects inside Lua
int lua_jni_callback_gc(lua_State *L) {
    jobject *ud = (jobject*)lua_touserdata(L, 1);
    if (ud && *ud) {
        JNIEnv *env = get_jni_env();
        if (env) {
            (*env)->DeleteGlobalRef(env, *ud);
        }
        *ud = NULL;
    }
    return 0;
}

// Generic C function invoked by Lua which routes back to Kotlin
int lua_jni_callback(lua_State *L) {
    jobject *ud = (jobject*)lua_touserdata(L, lua_upvalueindex(1));
    if (!ud || !*ud) return 0;
    jobject callback = *ud;

    JNIEnv *env = get_jni_env();
    if (!env || !callback_invoke_id || !luastate_wrap_id || !luastate_class) {
        return 0;
    }

    // Wrap the lua_State* pointer in a Kotlin LuaState wrapper
    jobject wrapped_state = (*env)->CallStaticObjectMethod(env, luastate_class, luastate_wrap_id, (jlong)L);
    if (!wrapped_state) return 0;

    // Invoke callback.invoke(wrapped_state)
    jint num_results = (*env)->CallIntMethod(env, callback, callback_invoke_id, wrapped_state);

    // Clean up the local reference to the temporary LuaState wrapper
    (*env)->DeleteLocalRef(env, wrapped_state);

    // Check if the Kotlin function threw an exception, and safely propagate it to Lua
    if ((*env)->ExceptionCheck(env)) {
        jthrowable ex = (*env)->ExceptionOccurred(env);
        (*env)->ExceptionClear(env);

        jclass ex_class = (*env)->GetObjectClass(env, ex);
        jmethodID to_string = (*env)->GetMethodID(env, ex_class, "toString", "()Ljava/lang/String;");
        jstring msg_str = (jstring)(*env)->CallObjectMethod(env, ex, to_string);
        const char *msg = (*env)->GetStringUTFChars(env, msg_str, NULL);

        lua_pushstring(L, msg ? msg : "Kotlin Exception");

        if (msg) {
            (*env)->ReleaseStringUTFChars(env, msg_str, msg);
        }
        (*env)->DeleteLocalRef(env, ex);
        (*env)->DeleteLocalRef(env, ex_class);
        (*env)->DeleteLocalRef(env, msg_str);

        return lua_error(L); // Triggers longjmp in Lua
    }

    return (int)num_results;
}

JNIEXPORT void JNICALL Java_io_github_anaruto_lua_LuaState_nativeRegisterFunction(JNIEnv *env, jobject thiz, jlong ptr, jstring name, jobject callback) {
    lua_State *L = (lua_State *)ptr;
    if (!L || !name || !callback || !luastate_class) return;

    // Create a new JVM global reference to persist the Kotlin callback object
    jobject g_callback = (*env)->NewGlobalRef(env, callback);

    // Allocate userdata in Lua to hold the JNI global reference
    jobject *ud = (jobject*)lua_newuserdatauv(L, sizeof(jobject), 0);
    *ud = g_callback;

    // Create or fetch the metatable for the callback userdata and assign the __gc method
    if (luaL_newmetatable(L, "LuaJniCallbackMeta")) {
        lua_pushcfunction(L, lua_jni_callback_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_setmetatable(L, -2); // Set metatable on userdata

    // Push C closure with the callback userdata as its upvalue
    lua_pushcclosure(L, lua_jni_callback, 1);

    // Bind the closure to the global name in Lua
    const char *func_name = (*env)->GetStringUTFChars(env, name, NULL);
    lua_setglobal(L, func_name);
    (*env)->ReleaseStringUTFChars(env, name, func_name);
}
