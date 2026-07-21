package lua

fun interface LuaCallback {
    fun invoke(L: LuaState): Int
}
