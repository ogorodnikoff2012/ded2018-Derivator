Derivator
===

**Derivator** is a small computer algebra system.
It allows you to evaluate mathematical expressions, simplify some of them and, surprizingly, take derivatives.

Build
---
Debug version:
```bash
mkdir build_debug && cd build_debug
cmake -DCMAKE_BUILD_TYPE=Debug <project-root-dir>
make
```

Release version:
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release <project-root-dir>
make
```
**Tip:** You can use `rlwrap ./repl` instead of `./repl` if you want to have GNU Readline features (history, navigation over input line, etc.)

Features
---

* Arithmetic operations: `(1 + 2) / 3 * 15 - 179 * (3 - 5)`
* Symbolic evaluation: `x + 2 * x + y * y - y * (x + 3) * y / (x + 3)` turns into `x * 3`
* Functions: `sin(3 * pi / 4) * log(exp(35))`
* Taking derivatives: `x'`, `x'_y`, `log(x * y)'_x'_y`
* Substitution: `(x * x * x - 6 * y + x * y * 8 + 3)(x, 3)(y, 5)` (syntax is quite strange, so in future versions it will be changed)
