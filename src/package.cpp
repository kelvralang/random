#include "NativePackageAPI.hpp"
#include <cstring>
#include <limits>
#include <random>
#include <string_view>
namespace {
struct R {
  std::mt19937_64 engine;
  explicit R(uint64_t s) : engine(s) {}
};
void err(ExprPackageStringView *e, const char *s) {
  if (e)
    *e = {s, strlen(s)};
}
bool seed(const ExprHostApi *, const ExprPackageValue *, size_t n,
          ExprPackageValue *r, ExprPackageStringView *e) {
  if (n) {
    err(e, "secureSeed expects no arguments");
    return false;
  }
  std::random_device d;
  r->kind = EXPR_PACKAGE_VALUE_U64;
  r->as.u64_value = (uint64_t(d()) << 32) ^ d();
  return true;
}
bool create(const ExprHostApi *, const ExprPackageValue *a, size_t n,
            ExprPackageValue *r, ExprPackageStringView *e) {
  if (n != 1 || a[0].kind != EXPR_PACKAGE_VALUE_U64) {
    err(e, "create expects a u64 seed");
    return false;
  }
  r->kind = EXPR_PACKAGE_VALUE_HANDLE;
  r->as.handle_value = {"github", "random", "RandomHandle",
                        new R(a[0].as.u64_value),
                        [](void *p) { delete static_cast<R *>(p); }};
  return true;
}
bool get(const ExprPackageValue &v, R *&o, ExprPackageStringView *e) {
  if (v.kind != EXPR_PACKAGE_VALUE_HANDLE || !v.as.handle_value.handle_data ||
      std::string_view(v.as.handle_value.package_name) != "random") {
    err(e, "expected github:random RandomHandle");
    return false;
  }
  o = static_cast<R *>(v.as.handle_value.handle_data);
  return true;
}
bool next(const ExprHostApi *, const ExprPackageValue *a, size_t n,
          ExprPackageValue *r, ExprPackageStringView *e) {
  R *x;
  if (n != 1 || !get(a[0], x, e))
    return false;
  r->kind = EXPR_PACKAGE_VALUE_U64;
  r->as.u64_value = x->engine();
  return true;
}
bool range(const ExprHostApi *, const ExprPackageValue *a, size_t n,
           ExprPackageValue *r, ExprPackageStringView *e) {
  R *x;
  if (n != 3 || !get(a[0], x, e) || a[1].kind != EXPR_PACKAGE_VALUE_I64 ||
      a[2].kind != EXPR_PACKAGE_VALUE_I64 ||
      a[1].as.i64_value > a[2].as.i64_value) {
    err(e, "nextI64 expects a Random and ordered i64 bounds");
    return false;
  }
  std::uniform_int_distribution<int64_t> d(a[1].as.i64_value,
                                           a[2].as.i64_value);
  r->kind = EXPR_PACKAGE_VALUE_I64;
  r->as.i64_value = d(x->engine);
  return true;
}
constexpr ExprPackageFunctionExport f[] = {
    {"secureSeed", "fn() -> u64", 0, seed},
    {"create", "fn(u64) -> handle<github:random:RandomHandle>", 1, create},
    {"nextU64", "fn(handle<github:random:RandomHandle>) -> u64", 1, next},
    {"nextI64", "fn(handle<github:random:RandomHandle>, i64, i64) -> i64", 3,
     range}};
constexpr ExprPackageRegistration x = {3, "github", "random", f, 4, nullptr, 0};
} // namespace
extern "C" const ExprPackageRegistration *exprRegisterPackage() { return &x; }
