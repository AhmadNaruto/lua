package lua

enum class LuaType(val id: Int) {
    NONE(-1),
    NIL(0),
    BOOLEAN(1),
    LIGHTUSERDATA(2),
    NUMBER(3),
    STRING(4),
    TABLE(5),
    FUNCTION(6),
    USERDATA(7),
    THREAD(8);

    companion object {
        fun fromId(id: Int): LuaType {
            return entries.firstOrNull { it.id == id } ?: NONE
        }
    }
}
