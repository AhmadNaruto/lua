package com.example.luaandroid

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import lua.LuaState
import lua.LuaException

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MaterialTheme(
                colorScheme = darkColorScheme(
                    primary = Color(0xFFBB86FC),
                    secondary = Color(0xFF03DAC6),
                    background = Color(0xFF121212),
                    surface = Color(0xFF1E1E1E),
                    onBackground = Color.White,
                    onSurface = Color.White
                )
            ) {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    LuaRunnerScreen()
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun LuaRunnerScreen() {
    var scriptText by remember {
        mutableStateOf(
            """-- Welcome to Lua 5.5 on Android!
-- You can write and run standard Lua scripts here.

print("Initializing Lua State...")

-- 1. Math operation
local a = 10
local b = 32
print("Result of " .. a .. " + " .. b .. " = " .. (a + b))

-- 2. Call Kotlin Native function!
-- 'kotlinLog' is registered from Kotlin
local doubleVal = kotlinLog(21)
print("Double of 21 returned from Kotlin: " .. doubleVal)
"""
        )
    }
    var consoleOutput by remember { mutableStateOf("Console output will appear here...\n") }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        Text(
            text = "Lua Interpreter on Android",
            fontSize = 22.sp,
            color = MaterialTheme.colorScheme.primary,
            fontFamily = FontFamily.SansSerif,
            modifier = Modifier.padding(bottom = 12.dp)
        )

        // Script Editor Card
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .weight(1f)
                .padding(bottom = 16.dp),
            colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface),
            elevation = CardDefaults.cardElevation(defaultElevation = 4.dp)
        ) {
            Box(modifier = Modifier.fillMaxSize().padding(8.dp)) {
                TextField(
                    value = scriptText,
                    onValueChange = { scriptText = it },
                    modifier = Modifier.fillMaxSize(),
                    textStyle = LocalTextStyle.current.copy(
                        fontFamily = FontFamily.Monospace,
                        fontSize = 14.sp,
                        color = Color(0xFFE2E2E2)
                    ),
                    colors = TextFieldDefaults.colors(
                        focusedContainerColor = Color.Transparent,
                        unfocusedContainerColor = Color.Transparent,
                        disabledContainerColor = Color.Transparent,
                        focusedIndicatorColor = Color.Transparent,
                        unfocusedIndicatorColor = Color.Transparent
                    )
                )
            }
        }

        // Action Row
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 16.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Button(
                onClick = {
                    consoleOutput = ""
                    val outputBuilder = StringBuilder()

                    try {
                        LuaState.create().use { lua ->
                            lua.openLibs()

                            // Define a custom print function routing to our UI output
                            lua.registerFunction("print") { L ->
                                val top = L.getTop()
                                for (i in 1..top) {
                                    outputBuilder.append(L.toString(i) ?: "nil").append(" ")
                                }
                                outputBuilder.append("\n")
                                0 // No return values on stack
                            }

                            // Define a callback showing Kotlin interop
                            lua.registerFunction("kotlinLog") { L ->
                                val input = L.toInteger(1)
                                L.pushInteger(input * 2)
                                1 // 1 return value on stack
                            }

                            lua.doString(scriptText)
                        }
                        consoleOutput = outputBuilder.toString()
                    } catch (e: LuaException) {
                        consoleOutput = "Lua Exception:\n${e.message}\n"
                    } catch (e: Exception) {
                        consoleOutput = "System Error:\n${e.message}\n"
                    }
                },
                modifier = Modifier.weight(1f),
                colors = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.primary)
            ) {
                Text("RUN SCRIPT", color = Color.Black)
            }

            OutlinedButton(
                onClick = {
                    scriptText = ""
                },
                colors = ButtonDefaults.outlinedButtonColors(contentColor = MaterialTheme.colorScheme.secondary)
            ) {
                Text("CLEAR")
            }
        }

        // Console Output label
        Text(
            text = "Console Output:",
            fontSize = 14.sp,
            color = MaterialTheme.colorScheme.secondary,
            modifier = Modifier.padding(bottom = 4.dp)
        )

        // Console Output Area
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(180.dp)
                .clip(RoundedCornerShape(8.dp))
                .background(Color(0xFF0C0C0C))
                .padding(12.dp)
                .verticalScroll(rememberScrollState())
        ) {
            Text(
                text = consoleOutput,
                fontFamily = FontFamily.Monospace,
                fontSize = 13.sp,
                color = Color(0xFF00FF66)
            )
        }
    }
}
