# Android Kotlin Binding for Lua

This project provides a native Kotlin binding for the Lua 5.5 scripting language, packaged as an Android Library module (`.aar`). 

---

## 1. Project Directory Structure

```
/android
├── gradle/
│   └── libs.versions.toml              # Dependency catalog (AGP, Kotlin, Compose)
├── app/
│   ├── build.gradle.kts                # NDK & Jetpack Compose build settings
│   ├── src/main/
│   │   ├── AndroidManifest.xml         # Android Library Manifest
│   │   ├── cpp/
│   │   │   ├── CMakeLists.txt          # NDK CMake build configuration
│   │   │   └── lua_jni.c               # Native JNI binding layer in C
│   │   └── kotlin/
│   │       ├── com/example/luaandroid/
│   │       │   └── MainActivity.kt     # Jetpack Compose Lua runner interface (Reference)
│   │       └── lua/
│   │           ├── LuaState.kt         # Core Kotlin context API
│   │           ├── LuaType.kt          # Lua type system maps
│   │           ├── LuaCallback.kt      # Interoperability callback interface
│   │           └── LuaException.kt     # Lua execution exception definition
│   └── README.md                       # This documentation file
├── build.gradle.kts                    # Root build script
├── settings.gradle.kts                 # Project settings & module mapping
└── gradle.properties                   # AndroidX properties configuration
```

---

## 2. Integration & How to Build

### A. Importing into Android Studio
1. Open Android Studio.
2. Select **File -> Open** and choose the `android` folder.
3. Android Studio will synchronize the Gradle project.
4. If building locally under Termux, run:
   ```bash
   ./gradlew build
   ```
   This compiles the library into an AAR, outputs are placed at `app/build/outputs/aar/app-release.aar`.

### B. Dependency Setup in Your APK Project
Copy the generated `app-release.aar` into your main application module's `libs` directory and update its `build.gradle.kts` dependencies:
```kotlin
dependencies {
    implementation(files("libs/app-release.aar"))
}
```

### C. NDK and Architecture Info
- **Host compiler**: Builds natively on Android-aarch64 environments using local compilers under the NDK folder (`29.0.14206865`).
- **Target ABI**: Restrained to target `arm64-v8a` (64-bit ARM architectures) specifically.

---

## 3. Complete API Documentation

### Class: `LuaState` (AutoCloseable)
Manages the native lifetime of a `lua_State*` pointer. It must be closed to prevent native memory leaks.

#### Lifecycle Methods
- **`companion object.create(): LuaState`**  
  Initializes a new native Lua execution context.
- **`close()`**  
  Safely destroys the Lua context and frees allocated memory.

#### Code Loading & Execution
- **`openLibs()`**  
  Loads all standard Lua libraries (`math`, `string`, `table`, `coroutine`, `package`, `io`).
- **`loadString(script: String)`**  
  Compiles a string of Lua script and pushes it as a function onto the stack.
- **`pcall(nargs: Int, nresults: Int, errfunc: Int = 0)`**  
  Calls a function in protected mode. Use `-1` for `nresults` to return all outputs.
- **`doString(script: String)`**  
  Combines `loadString` and `pcall` to load and immediately run a Lua script block.

#### Variable Scope
- **`getGlobal(name: String): LuaType`**  
  Pushes the global variable `name` onto the stack and returns its type.
- **`setGlobal(name: String)`**  
  Pops the top stack value and assigns it to the global variable `name`.

#### Stack Push
- **`pushNil()`**
- **`pushBoolean(value: Boolean)`**
- **`pushNumber(value: Double)`**
- **`pushInteger(value: Long)`**
- **`pushString(value: String)`**

#### Stack Read
- **`toBoolean(index: Int): Boolean`**
- **`toNumber(index: Int): Double`**
- **`toInteger(index: Int): Long`**
- **`toString(index: Int): String?`**
- **`type(index: Int): LuaType`**
- **`getTop(): Int`**
- **`pop(n: Int)`**

#### Table Operations
- **`newTable()`**: Pushes a new empty table onto the stack.
- **`getTable(index: Int): LuaType`**: Performs `table[key]` lookup where key is popped from the stack top.
- **`setTable(index: Int)`**: Performs `table[key] = value` popping key and value.
- **`rawGet(index: Int)`** & **`rawSet(index: Int)`**: Metatable-free variants of get and set.

#### Kotlin Native Callbacks
- **`registerFunction(name: String, callback: LuaCallback)`**  
  Binds a functional interface to a global name in Lua.

---

### Interface: `LuaCallback`
```kotlin
fun interface LuaCallback {
    fun invoke(L: LuaState): Int
}
```
- Returns the number of results pushed to the stack.
- Receives a temporary stack wrapper `L`. Do **not** call `L.close()` inside the callback.

---

### Enum Class: `LuaType`
Maps the native Lua type integers to a Kotlin enum:
- `NONE` (`-1`), `NIL` (`0`), `BOOLEAN` (`1`), `LIGHTUSERDATA` (`2`), `NUMBER` (`3`), `STRING` (`4`), `TABLE` (`5`), `FUNCTION` (`6`), `USERDATA` (`7`), `THREAD` (`8`).

---

### Exception: `LuaException`
Thrown on compilation failures or script runtime errors.

---

## 4. Usage Patterns & Code Examples

### Pattern 1: Execute script and read basic types
```kotlin
LuaState.create().use { lua ->
    lua.openLibs()
    
    // Evaluate math expression
    lua.loadString("return (10 * 3) + 12")
    lua.pcall(0, 1)
    
    val result = lua.toInteger(-1)
    println("Math Result: $result") // Math Result: 42
    
    lua.pop(1)
}
```

### Pattern 2: Interop with Kotlin lambdas (Callbacks)
```kotlin
LuaState.create().use { lua ->
    lua.openLibs()

    // Register a Kotlin function mapping to Lua global "kotlinCalc"
    lua.registerFunction("kotlinCalc") { L ->
        val num = L.toInteger(1)
        L.pushInteger(num * 10)
        1 // Return 1 value on the stack
    }

    lua.doString("print('Callback output:', kotlinCalc(5))") // Prints: Callback output: 50
}
```

### Pattern 3: Creating and reading Lua Tables
```kotlin
LuaState.create().use { lua ->
    // Create new table
    lua.newTable()
    
    // table["platform"] = "Android"
    lua.pushString("platform")
    lua.pushString("Android")
    lua.setTable(-3) // Table is located at index -3
    
    // table["version"] = 5.5
    lua.pushString("version")
    lua.pushNumber(5.5)
    lua.setTable(-3)
    
    // Assign global name
    lua.setGlobal("appConfig")
    
    // Read from global table
    lua.getGlobal("appConfig")
    lua.pushString("platform")
    lua.getTable(-2)
    
    val platformName = lua.toString(-1)
    println("Config platform: $platformName") // Config platform: Android
    
    lua.pop(2) // Clean up stack (value and table)
}
```
