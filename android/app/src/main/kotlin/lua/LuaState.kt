package lua

import java.io.Closeable

class LuaState private constructor(internal val ptr: Long, private val isOwner: Boolean) : Closeable {

    companion object {
        init {
            // Load the native library compiled by CMake
            System.loadLibrary("lua_jni")
        }

        /**
         * Creates and returns a new Lua state.
         */
        @JvmStatic
        fun create(): LuaState {
            val ptr = nativeNewState()
            if (ptr == 0L) {
                throw IllegalStateException("Failed to create Lua state")
            }
            return LuaState(ptr, isOwner = true)
        }

        /**
         * Internal helper to wrap an existing Lua state pointer (e.g. inside JNI callbacks).
         */
        @JvmStatic
        internal fun wrap(ptr: Long): LuaState {
            return LuaState(ptr, isOwner = false)
        }

        @JvmStatic
        private external fun nativeNewState(): Long
    }

    private var closed = false

    override fun close() {
        if (!closed) {
            closed = true
            if (isOwner && ptr != 0L) {
                nativeClose(ptr)
            }
        }
    }

    private fun checkClosed() {
        if (closed || ptr == 0L) {
            throw IllegalStateException("LuaState is closed or uninitialized")
        }
    }

    @Suppress("deprecation")
    protected fun finalize() {
        close()
    }

    fun openLibs() {
        checkClosed()
        nativeOpenLibs(ptr)
    }

    fun loadString(script: String) {
        checkClosed()
        val res = nativeLoadString(ptr, script)
        if (res != 0) {
            val errMsg = toString(-1) ?: "Unknown loading error"
            pop(1)
            throw LuaException("Failed to load script (error code $res): $errMsg")
        }
    }

    fun pcall(nargs: Int, nresults: Int, errfunc: Int = 0) {
        checkClosed()
        val res = nativePCall(ptr, nargs, nresults, errfunc)
        if (res != 0) {
            val errMsg = toString(-1) ?: "Unknown runtime error"
            pop(1)
            throw LuaException("Lua execution error (error code $res): $errMsg")
        }
    }

    fun doString(script: String) {
        loadString(script)
        pcall(0, -1) // -1 signifies LUA_MULTRET in our JNI mapping
    }

    fun getGlobal(name: String): LuaType {
        checkClosed()
        val typeId = nativeGetGlobal(ptr, name)
        return LuaType.fromId(typeId)
    }

    fun setGlobal(name: String) {
        checkClosed()
        nativeSetGlobal(ptr, name)
    }

    fun pushNil() {
        checkClosed()
        nativePushNil(ptr)
    }

    fun pushBoolean(value: Boolean) {
        checkClosed()
        nativePushBoolean(ptr, value)
    }

    fun pushNumber(value: Double) {
        checkClosed()
        nativePushNumber(ptr, value)
    }

    fun pushInteger(value: Long) {
        checkClosed()
        nativePushInteger(ptr, value)
    }

    fun pushString(value: String) {
        checkClosed()
        nativePushString(ptr, value)
    }

    fun toBoolean(index: Int): Boolean {
        checkClosed()
        return nativeToBoolean(ptr, index)
    }

    fun toNumber(index: Int): Double {
        checkClosed()
        return nativeToNumber(ptr, index)
    }

    fun toInteger(index: Int): Long {
        checkClosed()
        return nativeToInteger(ptr, index)
    }

    fun toString(index: Int): String? {
        checkClosed()
        return nativeToString(ptr, index)
    }

    fun type(index: Int): LuaType {
        checkClosed()
        return LuaType.fromId(nativeType(ptr, index))
    }

    fun getTop(): Int {
        checkClosed()
        return nativeGetTop(ptr)
    }

    fun pop(n: Int) {
        checkClosed()
        nativePop(ptr, n)
    }

    fun registerFunction(name: String, callback: LuaCallback) {
        checkClosed()
        nativeRegisterFunction(ptr, name, callback)
    }

    fun newTable() {
        checkClosed()
        nativeNewTable(ptr)
    }

    fun getTable(index: Int): LuaType {
        checkClosed()
        val typeId = nativeGetTable(ptr, index)
        return LuaType.fromId(typeId)
    }

    fun setTable(index: Int) {
        checkClosed()
        nativeSetTable(ptr, index)
    }

    fun rawGet(index: Int): LuaType {
        checkClosed()
        val typeId = nativeRawGet(ptr, index)
        return LuaType.fromId(typeId)
    }

    fun rawSet(index: Int) {
        checkClosed()
        nativeRawSet(ptr, index)
    }

    // JNI Native Methods
    private external fun nativeClose(ptr: Long)
    private external fun nativeOpenLibs(ptr: Long)
    private external fun nativeLoadString(ptr: Long, script: String): Int
    private external fun nativePCall(ptr: Long, nargs: Int, nresults: Int, errfunc: Int): Int
    private external fun nativeGetGlobal(ptr: Long, name: String): Int
    private external fun nativeSetGlobal(ptr: Long, name: String)
    private external fun nativePushNil(ptr: Long)
    private external fun nativePushBoolean(ptr: Long, value: Boolean)
    private external fun nativePushNumber(ptr: Long, value: Double)
    private external fun nativePushInteger(ptr: Long, value: Long)
    private external fun nativePushString(ptr: Long, value: String)
    private external fun nativeToBoolean(ptr: Long, index: Int): Boolean
    private external fun nativeToNumber(ptr: Long, index: Int): Double
    private external fun nativeToInteger(ptr: Long, index: Int): Long
    private external fun nativeToString(ptr: Long, index: Int): String?
    private external fun nativeType(ptr: Long, index: Int): Int
    private external fun nativeGetTop(ptr: Long): Int
    private external fun nativePop(ptr: Long, n: Int)
    private external fun nativeRegisterFunction(ptr: Long, name: String, callback: LuaCallback)
    private external fun nativeNewTable(ptr: Long)
    private external fun nativeGetTable(ptr: Long, index: Int): Int
    private external fun nativeSetTable(ptr: Long, index: Int)
    private external fun nativeRawGet(ptr: Long, index: Int): Int
    private external fun nativeRawSet(ptr: Long, index: Int)
}
