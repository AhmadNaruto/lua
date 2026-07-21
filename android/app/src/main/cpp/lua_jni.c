#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

// Cache for JVM and classes/methods
static JavaVM *cached_jvm = NULL;
static jclass luastate_class = NULL;
static jmethodID luastate_wrap_id = NULL;
static jmethodID callback_invoke_id = NULL;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
    cached_jvm = jvm;
    return JNI_VERSION_1_6;
}

JNIEnv *get_jni_env(void) {
    JNIEnv *env = NULL;
    jint res = (*cached_jvm)->GetEnv(cached_jvm, (void**)&env, JNI_VERSION_1_6);
    if (res == JNI_EDETACHED) {
        if ((*cached_jvm)->AttachCurrentThread(cached_jvm, &env, NULL) != 0) {
            return NULL;
        }
    }
    return env;
}

void init_jni_caches(JNIEnv *env) {
    if (luastate_class == NULL) {
        jclass local_class = (*env)->FindClass(env, "lua/LuaState");
        if (local_class) {
            luastate_class = (jclass)(*env)->NewGlobalRef(env, local_class);
            luastate_wrap_id = (*env)->GetStaticMethodID(env, luastate_class, "wrap", "(J)Llua/LuaState;");
        }
    }
    if (callback_invoke_id == NULL) {
        jclass cb_cls = (*env)->FindClass(env, "lua/LuaCallback");
        if (cb_cls) {
            callback_invoke_id = (*env)->GetMethodID(env, cb_cls, "invoke", "(Llua/LuaState;)I");
        }
    }
}

// ----------------------------------------------------------------------------
// Core lifecycle methods
// ----------------------------------------------------------------------------

JNIEXPORT jlong JNICALL Java_lua_LuaState_nativeNewState(JNIEnv *env, jclass clazz) {
    lua_State *L = luaL_newstate();
    return (jlong)L;
}

JNIEXPORT void JNICALL Java_lua_LuaState_nativeClose(JNIEnv *env, jobject thiz, jlong ptr) {
    lua_State *L = (lua_State *)ptr;
    if (L) {
        lua_close(L);
    }
}

JNIEXPORT void JNICALL Java_lua_LuaState_nativeOpenLibs(JNIEnv *env, jobject thiz, jlong ptr) {
    lua_State *L = (lua_State *)ptr;
    if (L) {
        luaL_openlibs(L);
    }
}

// ----------------------------------------------------------------------------
// Script execution methods
// ----------------------------------------------------------------------------

JNIEXPORT jint JNICALL Java_lua_LuaState_nativeLoadString(JNIEnv *env, jobject thiz, jlong ptr, jstring script) {
    lua_State *L = (lua_State *)ptr;
    if (!L || !script) return -1;
    const char *code = (*env)->GetStringUTFChars(env, script, NULL);
    int res = luaL_loadstring(L, code);
    (*env)->ReleaseStringUTFChars(env, script, code);
    return res;
}

JNIEXPORT jint JNICALL Java_lua_LuaState_nativePCall(JNIEnv *env, jobject thiz, jlong ptr, jint nargs, jint nresults, jint errfunc) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return -1;
    return lua_pcall(L, nargs, nresults, errfunc);
}

// ----------------------------------------------------------------------------
// Global variables
// ----------------------------------------------------------------------------

JNIEXPORT jint JNICALL Java_lua_LuaState_nativeGetGlobal(JNIEnv *env, jobject thiz, jlong ptr, jstring name) {
    lua_State *L = (lua_State *)ptr;
    if (!L || !name) return -1;
    const char *str = (*env)->GetStringUTFChars(env, name, NULL);
    int res = lua_getglobal(L, str);
    (*env)->ReleaseStringUTFChars(env, name, str);
    return res;
}

JNIEXPORT void JNICALL Java_lua_LuaState_nativeSetGlobal(JNIEnv *env, jobject thiz, jlong ptr, jstring name) {
    lua_State *L = (lua_State *)ptr;
    if (!L || !name) return;
    const char *str = (*env)->GetStringUTFChars(env, name, NULL);
    lua_setglobal(L, str);
    (*env)->ReleaseStringUTFChars(env, name, str);
}

// ----------------------------------------------------------------------------
// Push methods
// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_lua_LuaState_nativePushNil(JNIEnv *env, jobject thiz, jlong ptr) {
    lua_State *L = (lua_State *)ptr;
    if (L) lua_pushnil(L);
}

JNIEXPORT void JNICALL Java_lua_LuaState_nativePushBoolean(JNIEnv *env, jobject thiz, jlong ptr, jboolean val) {
    lua_State *L = (lua_State *)ptr;
    if (L) lua_pushboolean(L, val);
}

JNIEXPORT void JNICALL Java_lua_LuaState_nativePushNumber(JNIEnv *env, jobject thiz, jlong ptr, jdouble val) {
    lua_State *L = (lua_State *)ptr;
    if (L) lua_pushnumber(L, val);
}

JNIEXPORT void JNICALL Java_lua_LuaState_nativePushInteger(JNIEnv *env, jobject thiz, jlong ptr, jlong val) {
    lua_State *L = (lua_State *)ptr;
    if (L) lua_pushinteger(L, (lua_Integer)val);
}

JNIEXPORT void JNICALL Java_lua_LuaState_nativePushString(JNIEnv *env, jobject thiz, jlong ptr, jstring val) {
    lua_State *L = (lua_State *)ptr;
    if (!L || !val) return;
    const char *str = (*env)->GetStringUTFChars(env, val, NULL);
    lua_pushstring(L, str);
    (*env)->ReleaseStringUTFChars(env, val, str);
}

// ----------------------------------------------------------------------------
// Read methods
// ----------------------------------------------------------------------------

JNIEXPORT jboolean JNICALL Java_lua_LuaState_nativeToBoolean(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return JNI_FALSE;
    return lua_toboolean(L, index) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jdouble JNICALL Java_lua_LuaState_nativeToNumber(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return 0.0;
    return lua_tonumber(L, index);
}

JNIEXPORT jlong JNICALL Java_lua_LuaState_nativeToInteger(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return 0;
    return (jlong)lua_tointeger(L, index);
}

JNIEXPORT jstring JNICALL Java_lua_LuaState_nativeToString(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return NULL;
    const char *str = lua_tostring(L, index);
    if (!str) return NULL;
    return (*env)->NewStringUTF(env, str);
}

JNIEXPORT jint JNICALL Java_lua_LuaState_nativeType(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return -1;
    return lua_type(L, index);
}

JNIEXPORT jint JNICALL Java_lua_LuaState_nativeGetTop(JNIEnv *env, jobject thiz, jlong ptr) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return 0;
    return lua_gettop(L);
}

JNIEXPORT void JNICALL Java_lua_LuaState_nativePop(JNIEnv *env, jobject thiz, jlong ptr, jint n) {
    lua_State *L = (lua_State *)ptr;
    if (L) lua_pop(L, n);
}

// ----------------------------------------------------------------------------
// Table methods
// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_lua_LuaState_nativeNewTable(JNIEnv *env, jobject thiz, jlong ptr) {
    lua_State *L = (lua_State *)ptr;
    if (L) lua_newtable(L);
}

JNIEXPORT jint JNICALL Java_lua_LuaState_nativeGetTable(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return -1;
    return lua_gettable(L, index);
}

JNIEXPORT void JNICALL Java_lua_LuaState_nativeSetTable(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (L) lua_settable(L, index);
}

JNIEXPORT jint JNICALL Java_lua_LuaState_nativeRawGet(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
    lua_State *L = (lua_State *)ptr;
    if (!L) return -1;
    return lua_rawget(L, index);
}

JNIEXPORT void JNICALL Java_lua_LuaState_nativeRawSet(JNIEnv *env, jobject thiz, jlong ptr, jint index) {
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
    if (!env) return 0;

    init_jni_caches(env);
    if (!callback_invoke_id || !luastate_wrap_id || !luastate_class) {
        return 0;
    }

    // Wrap the lua_State* pointer in a Kotlin LuaState wrapper
    jobject wrapped_state = (*env)->CallStaticObjectMethod(env, luastate_class, luastate_wrap_id, (jlong)L);
    if (!wrapped_state) return 0;

    // Invoke callback.invoke(wrapped_state)
    jint num_results = (*env)->CallIntMethod(env, callback, callback_invoke_id, wrapped_state);

    // Clean up the local reference to the temporary LuaState wrapper
    (*env)->DeleteLocalRef(env, wrapped_state);

    return (int)num_results;
}

JNIEXPORT void JNICALL Java_lua_LuaState_nativeRegisterFunction(JNIEnv *env, jobject thiz, jlong ptr, jstring name, jobject callback) {
    lua_State *L = (lua_State *)ptr;
    if (!L || !name || !callback) return;

    init_jni_caches(env);

    // Create a new JVM global reference to persist the Kotlin callback object
    jobject g_callback = (*env)->NewGlobalRef(env, callback);

    // Allocate userdata in Lua to hold the JNI global reference (for automatic memory management)
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
