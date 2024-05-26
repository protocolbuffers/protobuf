#ifndef THIRD_PARTY_UPB_PROTOS_REQUIRES_H_
#define THIRD_PARTY_UPB_PROTOS_REQUIRES_H_

#include <type_traits>
namespace protos::internal {
// Ports C++20 `requires` to C++17.
// C++20 ideal:
//  if constexpr (requires { t.foo(); }) { ... }
// Our C++17 stopgap solution:
//  if constexpr (Requires<T>([](auto x) -> decltype(x.foo()) {})) { ... }
template <typename... T, typename F>
constexpr bool Requires(F) {
  return std::is_invocable_v<F, T...>;
}
}  // namespace protos::internal

#endif  // THIRD_PARTY_UPB_PROTOS_REQUIRES_H_
