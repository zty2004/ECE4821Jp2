#ifndef SRC_UTILS_FORMATTER_H_
#define SRC_UTILS_FORMATTER_H_

#include <string>
#include <vector>

template <typename T>
static inline auto to_string(const std::vector<T> &vec) -> std::string {
  std::string str;
  for (const auto &val : vec) {
    str += std::to_string(val) + " ";
  }
  return str;
}

template <typename T> static inline auto to_string(T value) -> std::string {
  /*
  check type first
  */
  if constexpr (std::is_same_v<T, std::string>) {
    return value;
  } else if constexpr (std::is_same_v<T, const char *>) {
    return std::string(value);
  } else {
    return std::to_string(value);
  }
}

template <typename T>
inline auto operator%(std::string format, T &&value) -> std::string {
  auto ind = format.find('?');
  if (ind == 0 || format[ind - 1] != '\\') {
    format.replace(ind, 1U, to_string(std::forward<T>(value)));
  }
  return format;
}

inline auto operator""_f(const char *str, size_t /*length*/) -> std::string {
  return std::string{str};
}

#endif // SRC_UTILS_FORMATTER_H_
