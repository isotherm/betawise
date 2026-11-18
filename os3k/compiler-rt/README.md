# OS3K compiler runtime

Betawise builds its compiler runtime into `libos3k.a`. The applet link
pulls in the archive members it needs and discards unused sections.
The Makefiles use the checked-in source files and need no network access
to build the runtime.

`upstream/` contains unmodified builtins and their headers from
[LLVM compiler-rt 21.1.0](https://github.com/llvm/llvm-project/releases/tag/llvmorg-21.1.0).
The source archive `compiler-rt-21.1.0.src.tar.xz` has SHA-256
`b065a2686674d931f2696517965260b1c92ffd18a492efb847fafb54457d690f`.
The upstream files retain their [Apache-2.0 license with the LLVM
exception](upstream/LICENSE.TXT). `builtins.mk` lists the selected sources.
They implement integer and bit operations and single- and double-precision
software floating point. Floating-point addition uses round-to-nearest-even.

OS3K supplies `__mulsi3`, a 32-bit product modulo 2³². A 68000 can
multiply two 16-bit halves into a 32-bit result with `MULU.W`. The helper
uses that instruction for the product of the low halves and the two cross
products. The product of the high halves would start at bit 32. Explicit
assembly pins the three word multiplies because compiling those products
as C can make GCC emit calls back to `__mulsi3`.

Run the arithmetic smoke test from the Betawise root with QEMU user mode
installed.

```sh
make -C os3k -j4
os3k/compiler-rt/check.sh
```
