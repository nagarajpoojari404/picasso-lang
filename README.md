# Picasso Programming Language

Picasso is a modern, compiled programming language designed for myself.

## Overview

Picasso combines the performance of compiled languages with the ease of use of modern high-level languages. It features automatic memory management, built-in concurrency primitives, and a rich standard library while maintaining zero-cost abstractions.

## Table of Contents

- [Key Features](#key-features)
- [Installation via Homebrew](#installation-via-homebrew)
- [Building from Source](#building-from-source)
- [Syntax Examples](#syntax-examples)
- [Variable Declaration](#variable-declaration)
- [Access Modifiers](#access-modifiers)
- [Built-in Libraries](#built-in-libraries)
- [Citation](#citation)

## Key Features

- **Compiled Native Code**: Direct compilation to native machine code without virtual machine overhead.

- **Procedural with Object Support**: Clean procedural programming with full support for classes and objects.

- **Rich Type System**: Signed/unsigned integers (`int8` to `int64`, `uint8` to `uint64`), floating point (`float`, `double`), strings, atomics, dynamic arrays, and user-defined classes.

- **Built-in Concurrency**: Lightweight green threads with `thread()` function - no explicit async/await required. Scale to hundreds of thousands of concurrent tasks.

- **Rich set of synchronization primitives**: `waitgroups`, `(* *)` for non-preemptive code blocks.
  
- **Automatic Memory Management**: Garbage collected runtime - allocate and forget.

- **C Interoperability**: Foreign Function Interface (FFI) for seamless integration with C libraries.

- **Modular Design**: Simple module system with `using` statements and clear namespace separation.

- **Cross-Platform Support**: Linux and macOS on aarch64/arm64 architectures.

- **Comprehensive Standard Library**: Network I/O, file I/O, OS integration, synchronization primitives, string manipulation, and array operations.

---

## Installation via Homebrew

The easiest way to install Picasso on macOS (Apple Silicon / arm64).

### Prerequisites

- macOS with Apple Silicon (arm64)
- [Homebrew](https://brew.sh) installed
- Xcode Command Line Tools — install or update with:
  ```zsh
  xcode-select --install
  ```

### Install

```zsh
brew install nagarajRPoojari/picasso/picasso
```

This single command taps the formula, downloads the pre-built binary, and places all components in the right locations:

| Component | Installed path |
|---|---|
| `picasso` CLI | `/opt/homebrew/bin/picasso` |
| `irgen` (IR generator) | `/opt/homebrew/bin/irgen` |
| Runtime library | `/opt/homebrew/lib/picasso/libruntime_lib.a` |
| Standard library headers | `/opt/homebrew/lib/picasso/libs/` |
| Runtime headers | `/opt/homebrew/lib/picasso/runtime/` |

### Verify

```zsh
picasso --help
```

### Upgrade

```zsh
brew upgrade nagarajRPoojari/picasso/picasso
```

### Uninstall

```zsh
brew uninstall picasso
brew untap nagarajRPoojari/picasso   # optional, removes the tap
```

---

## Building from Source

> **Platform:** macOS (arm64 / Apple Silicon)

### Prerequisites

| Tool | Version | Install |
|---|---|---|
| Xcode Command Line Tools | latest | `xcode-select --install` |
| Go | ≥ 1.24.5 | `brew install go` |
| libffi | any | `brew install libffi` |
| Bazelisk | latest | `brew install bazelisk` |

Bazelisk automatically selects **Bazel 8.5.1** (pinned in `.bazelversion`). Always run Bazel commands from inside the `picasso/` directory so Bazelisk picks up the correct version.

### 1. Clone and enter the workspace

```bash
git clone https://github.com/nagarajRPoojari/picasso.git
cd picasso
```

### 2. Build

```bash
bazel build //cli:picasso
```

This compiles three components in one step:

| Output | Path under `bazel-bin/` | Description |
|---|---|---|
| `picasso` | `cli/picasso` | Compiler CLI |
| `irgen` | `irgen/irgen_/irgen` | IR generator (Go) |
| `libruntime_lib.a` | `libruntime_lib.a` | Runtime static library (C) |

### 3. Package

```bash
./build.sh v1.0.2
# → dist/picasso_v1.0.2_darwin_arm64.tar.gz
```

### 4. Install

```bash
VERSION="v1.0.2"
PKG="dist/picasso_${VERSION}_darwin_arm64"

sudo cp "$PKG/picasso"          /usr/local/bin/picasso
sudo cp "$PKG/irgen"            /usr/local/bin/irgen

sudo mkdir -p /usr/local/lib/picasso
sudo cp    "$PKG/libruntime_lib.a"  /usr/local/lib/picasso/
sudo cp -R "$PKG/libs"              /usr/local/lib/picasso/
sudo cp -R "$PKG/runtime"           /usr/local/lib/picasso/
```

### 5. Verify

```bash
picasso --help
```

### Troubleshooting

| Error | Cause | Fix |
|---|---|---|
| Bazel downloads wrong version (e.g. 9.x) | Running `bazel` from the wrong directory | `cd picasso` first; Bazelisk reads `.bazelversion` only from the cwd |
| Stale Go toolchain path after `brew upgrade` | Bazel cached a path that no longer exists | `bazel clean --expunge` then rebuild |
| `go: unknown GOEXPERIMENT coverageredesign` | `go_sdk.host()` pairs a new system Go with rules_go's old builder binary | Use `go_sdk.download(version = "1.24.5")` in `MODULE.bazel` so rules_go downloads a matched SDK |
| `ld: library 'ffi' not found` | libffi not installed | `brew install libffi` |
| `fatal error: 'signal.h' file not found` | macOS SDK path changed after a CLT/Xcode update; LLVM 14 can't locate system headers | See below |

#### `signal.h` not found after CLT update

This occurs when the macOS Command Line Tools or Xcode are updated and the SDK path shifts — LLVM 14's clang no longer finds standard system headers automatically.

**Permanent fix (already baked into the binary as of v1.0.7):** `picasso` now resolves the SDK via `xcrun --sdk macosx --show-sdk-path` at build time and passes `-isysroot` to clang automatically.

**Workaround for older installed versions:** set `SDKROOT` before running `picasso`:

```zsh
export SDKROOT=$(xcrun --show-sdk-path)
picasso build <project-dir>
```

You can also pin a specific SDK path permanently:

```zsh
export PICASSO_SDK_PATH=$(xcrun --show-sdk-path)
picasso build <project-dir>
```

Add either export to your `~/.zshrc` to make it persistent. The recommended fix is to upgrade to the latest `picasso` release via `brew upgrade nagarajRPoojari/picasso/picasso`.

---

## Syntax Examples

### Hello World

```picasso
using "builtin/syncio";

fn start(args: []string) {
    syncio.printf("Hello, World!\n");
}
```

### Classes and Objects

```python
using "builtin/syncio";

class Person {
    say name: string;
    say age: int;

    fn Person(name: string, age: int) {
        this.name = name;
        this.age = age;
    }

    fn greet() {
        syncio.printf("Hello, I'm %s and I'm %d years old\n", this.name, this.age);
    }
}

fn start(args: []string) {
    say person: start.Person = new start.Person("Alice", 30);
    person.greet();
}
```

### Control Flow

```python
using "builtin/syncio";

fn start(args: []string) {
    say x: int = 10;
    
    if (x < 0) {
        syncio.printf("Negative\n");
    } else if (x == 0) {
        syncio.printf("Zero\n");
    } else {
        syncio.printf("Positive\n");
    }
    
    // While loop
    say i: int = 0;
    while (i < 5) {
        syncio.printf("%d ", i);
        i = i + 1;
    }
    
    // Foreach loop
    foreach j in 0..10 {
        syncio.printf("%d ", j);
    }
}
```

### Arrays

```python
using "builtin/syncio";
using "builtin/array";

fn start(args: []string) {
    say numbers: []int = array.create(int, 5);
    
    foreach i in 0..array.len(numbers) {
        numbers[i] = i * 10;
    }
    
    array.append(numbers, 50);
    array.append(numbers, 60);
    
    foreach i in 0..array.len(numbers) {
        syncio.printf("numbers[%d] = %d\n", i, numbers[i]);
    }
}
```

### Concurrency

```python
using "builtin/syncio";

class Worker {
    say id: int;
    
    fn Worker(id: int) {
        this.id = id;
    }
    
    fn work() {
        syncio.printf("Worker %d is working\n", this.id);
    }
}

fn start(args: []string) {
    foreach i in 0..10 {
        say worker: start.Worker = new start.Worker(i);
        thread(worker.work);
    }
}
```

### Synchronization
```java
using "builtin/syncio";
using "builtin/sync";
using "builtin/array";

fn start(args: []string) {
    say wg: sync.waitgroup = sync.waitgroup_create();
    
    say shared_counter: int = 0;
    say num_workers: int = 100;

    sync.waitgroup_add(wg, num_workers);
    syncio.printf("Starting %d workers...\n", num_workers);

    foreach i in 0..num_workers {
        thread(fn() {
            // --- CRITICAL SECTION ---
            // We use the non-preemptive block to safely increment 
            // the shared counter without a Mutex.
            (*
                shared_counter = shared_counter + 1;
            *)
            // ------------------------

            // Signal task completion
            sync.waitgroup_done(wg);
        });
    }

    sync.waitgroup_wait(wg);

    syncio.printf("Final Counter Value: %d\n", shared_counter);
    syncio.printf("All threads synchronized successfully.\n");
}

```

### Atomic Operations

```python
using "builtin/syncio";
using "builtin/atomics";

fn start(args: []string) {
    say counter: atomic int64;
    
    atomics.store_int64(counter, int64(0));
    atomics.add_int64(counter, int64(10));
    atomics.sub_int64(counter, int64(3));
    
    say value: int64 = atomics.load_int64(counter);
    syncio.printf("Counter value: %ld\n", value);
}
```

### Network Programming

```python
using "builtin/syncio";
using "builtin/net";
using "builtin/array";

class Server {
    say addr: string;
    say port: int16;
    
    fn Server(addr: string, port: int16) {
        this.addr = addr;
        this.port = port;
    }
    
    fn start(args: []string) {
        say fd: int = net.listen(this.addr, this.port, 4096, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0);
        
        if (fd < 0) {
            syncio.printf("Failed to start server\n");
            return;
        }
        
        syncio.printf("Server listening on %s:%d\n", this.addr, this.port);
        
        say clientFd: int = net.accept(fd);
        say buf: []uint8 = array.create(uint8, 1024);
        say n: int = net.read(clientFd, buf, 1024);
        
        if (n > 0) {
            net.write(clientFd, buf, n);
        }
    }
}

fn start(args: []string) {
    say server: start.Server = new start.Server("127.0.0.1", 8080);
    server.start();
}
```

### File I/O

```python
using "builtin/syncio";
using "builtin/array";

fn start(args: []string) {
    say file: string = syncio.fopen("data.txt", "w+");
    
    say data: []uint8 = array.create(uint8, 10);
    foreach i in 0..array.len(data) {
        data[i] = i;
    }
    
    syncio.fwrite(file, data, array.len(data), 0);
    
    say readBuf: []uint8 = array.create(uint8, 10);
    syncio.fread(file, readBuf, 10, 0);
    
    syncio.fclose(file);
}
```

### Module System

```python
// math.pic
using "builtin/syncio";

class Calculator {
    fn Calculator() {}
    
    fn add(a: int, b: int): int {
        return a + b;
    }
}
```

```python
// start.pic
using "builtin/syncio";
using "math" as m;

fn start(args: []string) {
    say calc: m.Calculator = new m.Calculator();
    say result: int = calc.add(5, 3);
    syncio.printf("Result: %d\n", result);
}
```

## Variable Declaration

Variables are declared using the `say` keyword:

```python
say x: int = 10;
say name: string = "Alice";
say numbers: []int = array.create(int, 5);
say person: start.Person = new start.Person("Bob", 25);
```

## Access Modifiers

- **Public fields/methods**: Use `say` keyword (accessible from other modules)
- **Internal fields/methods**: Use `say internal` keyword (module-private)

```python
class Example {
    say publicField: int;
    say internal privateField: int;
    
    fn Example() {}
    
    fn publicMethod() {}
    
    fn internal privateMethod() {}
}
```

## Built-in Libraries

- **syncio**: Synchronous I/O operations including console output and file operations.
- **net**: Network programming with TCP sockets, client/server support.
- **array**: Dynamic array operations including creation, length, and append.
- **strings**: String manipulation utilities including formatting, comparison, and substring operations.
- **atomics**: Lock-free atomic operations for concurrent programming.
- **types**: Type conversion and type-related utilities.


---

## Citation

```bibtex
@software{Poojari_Picasso_A_lightweight_2026,
  author  = {Poojari, Nagaraj},
  doi     = {10.5281/zenodo.20106946.},
  month   = may,
  title   = {{Picasso: A lightweight programming language for modern workloads}},
  url     = {https://github.com/nagarajRPoojari/picasso},
  version = {1.0.0},
  year    = {2026}
}
```
