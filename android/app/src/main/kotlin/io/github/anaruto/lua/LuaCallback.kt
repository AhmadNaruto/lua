package io.github.anaruto.lua

fun interface LuaCallback {
    fun invoke(L: LuaState): Int
}
