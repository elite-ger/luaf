# Luaf

A lightweight, sandboxed scripting runtime with cooperative multitasking.

Luaf extends Lua 5.5.0 with `wait()`, `spawn()`, `delay()`, async HTTP,
and a Python-like module system — designed as a scripting foundation for
game engines and creative tools.

## Features

- **Cooperative multitasking** — `wait()`, `spawn()`, `delay()` for parallel scripts without OS threads
- **Async HTTP** — non-blocking `http.get_async()` with callbacks
- **Solid variables** — `solid("name", value)` for auto-syncing state (ready for networking)
- **Vector math** — `Vector1`, `Vector2`, `Vector3`, `Vector4` with full operator overloading
- **Module system** — `require("name")` loads from `modules/name.lua` (Python-style)
- **Sandboxed** — per-script memory and instruction limits, dangerous functions blocked
- **Lua 5.5.0** — latest Lua with safe standard libraries
- **Embeddable** — build as a static library and link into your engine
- **REPL** — interactive console for quick scripting
- **MIT licensed** — use it anywhere

## Quick Start

### Building

```bash
cmake -B build -S .
cmake --build build
```

Requires a C compiler and CMake 3.16+. On Windows, links against `ws2_32` for HTTP.

### Interactive REPL

```bash
./build/luaf
```

```
Luaf v0.1.0 (Lua 5.5.0 + wait/spawn/delay/http)
Type 'exit' or Ctrl+D to quit.

luaf> print("Hello!")
Hello!
luaf> spawn(function() for i=1,3 do print(i) wait(0.5) end end)
1
2
3
luaf> exit
```

### Run a script

```bash
./build/luaf examples/test_all.lua
```

### Execute code directly

```bash
./build/luaf -e "print(2 + 2)"
```

## Luaf API

### Core

| Function | Description |
|----------|-------------|
| `wait(seconds)` | Yield execution for a duration (default: 1/30s) |
| `spawn(func)` | Run a function in a new parallel thread |
| `delay(seconds, func)` | Run a function after a delay |
| `solid(name, value)` | Create an auto-syncing variable |
| `require("name")` | Load a module from `modules/` |

### HTTP

| Function | Description |
|----------|-------------|
| `http.get(url)` | Synchronous GET request |
| `http.post(url, body)` | Synchronous POST request |
| `http.get_async(url, callback)` | Async GET, calls `callback(data, err)` when done |

### Vectors

```lua
local v = Vector3.new(10, 20, 30)
print(v + v)            -- 20, 40, 60
print(v:Magnitude())    -- 37.41...
print(v:Unit())         -- 0.267, 0.535, 0.802
print(v:Dot(other))     -- dot product
print(v:Cross(other))   -- cross product (Vector3 only)
print(v:Lerp(target, t)) -- linear interpolation
```

All vectors (`Vector1`, `Vector2`, `Vector3`, `Vector4`) support:
- Arithmetic: `+`, `-`, `*`, `/`, `-` (unary)
- Comparison: `==`, `<`, `<=`
- Methods: `Magnitude()`, `Unit()`, `Dot()`, `Lerp()`, `Abs()`, `Clone()`

### Solid Variables

```lua
solid("health", 100)
print(health.value)     -- 100
health.value = 50       -- automatically broadcasts via Net module
health.value = health.value + 10  -- arithmetic works
```

### Signals

```lua
local Signal = require("signal")
local sig = Signal.new()
sig:Connect(function(msg) print(msg) end)
sig:Fire("Hello!")      -- prints "Hello!"
```

### Tween

```lua
local Tween = require("tween")
local Easing = require("easing")

local obj = { X = 0 }
local tw = Tween.new(obj, { X = 100 }, 1, Easing.ElasticOut)
tw:Play()
tw:Update(0.5)  -- animate halfway
```

## Built-in Modules

| Module | Description |
|--------|-------------|
| `signal` | Event system (Connect/Fire/Disconnect) |
| `easing` | Animation easing functions (Quad, Cubic, Sine, Elastic, Bounce, Back) |
| `tween` | Property animation system |
| `world` | 3D world management (for Freeblocks engine) |
| `input` | Keyboard/mouse input |
| `time` | Delta time and total elapsed time |
| `run` | Heartbeat/Stepped/RenderStepped events |
| `debris` | Delayed object destruction |
| `net` | Network event broadcasting |
| `store` | Key-value data storage |
| `assets` | Texture/sound/model loading |
| `camera` | Camera position and direction |
| `game` | Game lifecycle (Start/End/Pause/Reset) |

## Embedding

```c
#include "luaf_runtime.h"

LuafRuntime* rt = luaf_runtime_new();
luaf_runtime_add_file(rt, "game.lua");

while (luaf_runtime_has_tasks(rt)) {
    double dt = get_frame_delta();
    luaf_runtime_tick(rt, dt);
    render_frame();
}

luaf_runtime_free(rt);
```

## Project Structure

```
luaf/
├── lib/lua-5.5.0/src/   # Original Lua 5.5.0 (unmodified)
├── src/                  # Luaf core
│   ├── luaf_state.c      # Sandboxed Lua state
│   ├── luaf_sandbox.c    # Safe function whitelist
│   ├── luaf_runtime.c    # Scheduler (wait/spawn/delay)
│   ├── luaf_http.c       # HTTP client (sync + async)
│   ├── luaf_vector.c     # Vector registration
│   └── luaf_dofile.c     # Script loading
├── modules/              # Lua modules (Python-style imports)
└── host/main.c           # REPL and CLI
```

## License

MIT — see [LICENSE](LICENSE) for details.

Uses Lua 5.5.0, copyright Lua.org, PUC-Rio, also MIT licensed.
