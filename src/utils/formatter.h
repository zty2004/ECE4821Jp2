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

// t is too short, use value instead

template <typename T> static inline auto to_string(T tt) -> std::string {
  return std::to_string(tt);
}

template <typename T>
inline auto operator%(std::string format, T value) -> std::string {
  auto ind = format.find('?');
  if (ind == 0 || format[ind - 1] != '\\') {
    format.replace(ind, 1U, to_string(value));
  }
  return format;
}

template <>
inline auto operator%(std::string format,
                      std::string value) // clang-tidy unfixed, keep compilation
    -> std::string {
  auto ind = format.find('?');
  if (ind == 0 || format[ind - 1] != '\\') {
    format.replace(ind, 1U, value);
  }
  return format;
}

template <>
inline auto operator%(std::string format, const char *value) -> std::string {
  auto ind = format.find('?');
  if (ind == 0 || format[ind - 1] != '\\') {
    format.replace(ind, 1U, value);
  }
  return format;
}

inline auto operator""_f(const char *str, size_t /*length*/) -> std::string {
  return std::string{str};
}

#endif // SRC_UTILS_FORMATTER_H_
