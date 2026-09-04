# kelvralang/random

Seedable pseudo-random number generation for Kelvra. The canonical import is
`github.com/kelvralang/random`. This native package supports ABI 3 and Kelvra runtime
`^0.2.0` on Linux x86_64, Linux ARM64, and macOS ARM64.

Install from a Kelvra project directory. Git dependencies build from source and
therefore require CMake and a C++17 compiler:

```bash
kelvra add github.com/kelvralang/random@v0.2.0
```

```kelvra
const random = @import("github.com/kelvralang/random")

var generator random.Random = random.create(random.secureSeed())
var dieRoll i64 = random.nextI64(generator, 1, 6)
var fraction f64 = random.nextF64(generator) // 0.0 <= value < 1.0
var heads bool = random.nextBool(generator)
```

`create` uses the deterministic MT19937-64 algorithm, so equal seeds produce the
same `nextU64` sequence. The C++ standard does not require bounded and floating
distributions to produce identical sequences across standard-library
implementations. This generator is suitable for simulations and games, not
cryptographic output. `secureSeed` obtains a seed from the operating system CSPRNG
(`getrandom` on Linux and `arc4random_buf` on macOS), but values subsequently
produced by a `Random` remain predictable to anyone who learns its state or seed.

`nextI64` includes both bounds. `nextF64` returns a value in `[0.0, 1.0)`.

Build with CMake. The complete public contract is in `package.api.kel`. The
package is licensed under GPL-3.0-only; see `LICENSE`.
