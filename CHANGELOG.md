# Changelog

## 0.2.0

- Rename package manifests, source files, imports, automation, and documentation from Mog to Kelvra; require Kelvra 0.2.0 or newer.

- Correct the minimum supported runtime to Kelvra 0.1.4, the first release that
  embeds its configured package-compatibility version correctly.
- Add pinned multi-target CI/release automation with tag checks, runtime tests,
  checksummed native archives, and automated action updates.
- Enforce portable C++17 mode in native builds.
- Remove the unsupported macOS x86_64 target from release metadata.
- Correct the manifest license identifier to `GPL-3.0-only` to match `LICENSE`.
- Source `secureSeed` directly from the operating system CSPRNG on declared Linux
  and macOS targets instead of relying on `std::random_device`.
- Validate the complete native handle namespace, package, and type identity.
- Add deterministic `nextBool` and `nextF64` helpers and expand tests.

## 0.1.1

- Require Kelvra runtime 0.1.1 or newer for local native package loading.

## 0.1.0

- Initial foundation release.
