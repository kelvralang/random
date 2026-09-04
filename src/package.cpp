#include "NativePackageAPI.hpp"
#include <cerrno>
#include <cstring>
#include <limits>
#include <random>
#include <string>
#include <string_view>

#if defined(__linux__)
#include <sys/random.h>
#elif defined(__APPLE__)
#include <stdlib.h>
#else
#error "kelvralang/random secureSeed supports only declared Linux and macOS targets"
#endif

namespace {

struct R {
  std::mt19937_64 engine;
  explicit R(uint64_t s) : engine(s) {}
};

void err(ExprPackageStringView *e, const char *s) {
  if (e)
    *e = {s, strlen(s)};
}

void err(ExprPackageStringView *e, const std::string &s) {
  if (!e)
    return;
  static thread_local std::string message;
  message = s;
  *e = {message.c_str(), message.size()};
}

bool fillSecure(void *destination, size_t size, ExprPackageStringView *e) {
#if defined(__linux__)
  auto *bytes = static_cast<unsigned char *>(destination);
  size_t offset = 0;
  while (offset < size) {
    const ssize_t count = ::getrandom(bytes + offset, size - offset, 0);
    if (count > 0) {
      offset += static_cast<size_t>(count);
      continue;
    }
    if (count < 0 && errno == EINTR)
      continue;
    const int code = count < 0 ? errno : EIO;
    err(e, "secureSeed could not read operating-system entropy: " +
               std::string(std::strerror(code)));
    return false;
  }
#elif defined(__APPLE__)
  ::arc4random_buf(destination, size);
#endif
  return true;
}

bool seed(const ExprHostApi *, const ExprPackageValue *, size_t n,
          ExprPackageValue *r, ExprPackageStringView *e) {
  if (n) {
    err(e, "secureSeed expects no arguments");
    return false;
  }
  uint64_t value = 0;
  if (!fillSecure(&value, sizeof(value), e))
    return false;
  r->kind = EXPR_PACKAGE_VALUE_U64;
  r->as.u64_value = value;
  return true;
}

bool create(const ExprHostApi *, const ExprPackageValue *a, size_t n,
            ExprPackageValue *r, ExprPackageStringView *e) {
  if (n != 1 || !a || a[0].kind != EXPR_PACKAGE_VALUE_U64) {
    err(e, "create expects a u64 seed");
    return false;
  }
  r->kind = EXPR_PACKAGE_VALUE_HANDLE;
  r->as.handle_value = {"github", "random", "RandomHandle",
                        new R(a[0].as.u64_value),
                        [](void *p) { delete static_cast<R *>(p); }};
  return true;
}

bool same(const char *actual, const char *expected) {
  return actual && std::string_view(actual) == expected;
}

bool get(const ExprPackageValue &v, R *&o, ExprPackageStringView *e) {
  if (v.kind != EXPR_PACKAGE_VALUE_HANDLE || !v.as.handle_value.handle_data ||
      !same(v.as.handle_value.package_namespace, "github") ||
      !same(v.as.handle_value.package_name, "random") ||
      !same(v.as.handle_value.type_name, "RandomHandle")) {
    err(e, "expected github:random RandomHandle");
    return false;
  }
  o = static_cast<R *>(v.as.handle_value.handle_data);
  return true;
}
bool next(const ExprHostApi *, const ExprPackageValue *a, size_t n,
          ExprPackageValue *r, ExprPackageStringView *e) {
  R *x;
  if (n != 1 || !a) {
    err(e, "nextU64 expects a Random");
    return false;
  }
  if (!get(a[0], x, e))
    return false;
  r->kind = EXPR_PACKAGE_VALUE_U64;
  r->as.u64_value = x->engine();
  return true;
}
bool range(const ExprHostApi *, const ExprPackageValue *a, size_t n,
           ExprPackageValue *r, ExprPackageStringView *e) {
  R *x;
  if (n != 3 || !a || a[1].kind != EXPR_PACKAGE_VALUE_I64 ||
      a[2].kind != EXPR_PACKAGE_VALUE_I64) {
    err(e, "nextI64 expects a Random and ordered i64 bounds");
    return false;
  }
  if (!get(a[0], x, e))
    return false;
  if (a[1].as.i64_value > a[2].as.i64_value) {
    err(e, "nextI64 minimum must be less than or equal to maximum");
    return false;
  }
  std::uniform_int_distribution<int64_t> d(a[1].as.i64_value,
                                           a[2].as.i64_value);
  r->kind = EXPR_PACKAGE_VALUE_I64;
  r->as.i64_value = d(x->engine);
  return true;
}

bool nextBool(const ExprHostApi *, const ExprPackageValue *a, size_t n,
              ExprPackageValue *r, ExprPackageStringView *e) {
  R *x;
  if (n != 1 || !a) {
    err(e, "nextBool expects a Random");
    return false;
  }
  if (!get(a[0], x, e))
    return false;
  r->kind = EXPR_PACKAGE_VALUE_BOOL;
  r->as.boolean_value = (x->engine() & uint64_t{1}) != 0;
  return true;
}

bool nextF64(const ExprHostApi *, const ExprPackageValue *a, size_t n,
             ExprPackageValue *r, ExprPackageStringView *e) {
  R *x;
  if (n != 1 || !a) {
    err(e, "nextF64 expects a Random");
    return false;
  }
  if (!get(a[0], x, e))
    return false;
  r->kind = EXPR_PACKAGE_VALUE_F64;
  r->as.f64_value = std::generate_canonical<double,
                                             std::numeric_limits<double>::digits>(
      x->engine);
  return true;
}

constexpr ExprPackageFunctionExport f[] = {
    {"secureSeed", "fn() -> u64", 0, seed},
    {"create", "fn(u64) -> handle<github:random:RandomHandle>", 1, create},
    {"nextU64", "fn(handle<github:random:RandomHandle>) -> u64", 1, next},
    {"nextI64", "fn(handle<github:random:RandomHandle>, i64, i64) -> i64", 3,
     range},
    {"nextBool", "fn(handle<github:random:RandomHandle>) -> bool", 1,
     nextBool},
    {"nextF64", "fn(handle<github:random:RandomHandle>) -> f64", 1,
     nextF64}};
constexpr ExprPackageRegistration x = {3, "github", "random", f, 6, nullptr, 0};
} // namespace
extern "C" const ExprPackageRegistration *exprRegisterPackage() { return &x; }
