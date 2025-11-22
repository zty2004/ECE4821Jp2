//
// Argument parser implementation
//

#include "ArgParser.h"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

namespace {
const int dec = 10;

inline auto to_sv(char *param) -> std::string_view {
  return (param != nullptr) ? std::string_view(param) : std::string_view();
}

inline void warn_unknown(const std::string_view &token) {
  std::cerr << "lemondb: warning: unknown argument " << token << '\n';
}

inline void warn_missing(const std::string_view &opt) {
  std::cerr << "lemondb: warning: missing value for " << opt << '\n';
}

inline void warn_invalid_threads(const std::string_view &value) {
  std::cerr << "lemondb: warning: invalid value for --threads " << value
            << '\n';
}

inline auto parse_int64_sv(std::string_view sv) -> std::optional<int64_t> {
  int64_t parsed{};
  const char *first = sv.data();
  const char *last = std::next(first, static_cast<std::ptrdiff_t>(sv.size()));
  auto res = std::from_chars(first, last, parsed, dec);
  if (res.ec == std::errc() && res.ptr == last) {
    return parsed;
  }
  return std::nullopt;
}

inline void handle_long_option(size_t *index, std::span<char *> argv,
                               size_t num, std::string_view token,
                               ParsedArgs *out) {
  const std::string_view rest = token.substr(2);
  const size_t eq_pos = rest.find('=');
  const std::string_view name = rest.substr(0, eq_pos);
  std::string_view value_sv;
  bool has_value = false;
  if (eq_pos != std::string_view::npos) {
    value_sv = rest.substr(eq_pos + 1);
    has_value = true;
  }

  auto require_value = [&](std::string_view optname) -> std::string_view {
    if (has_value) {
      return value_sv;
    }
    if ((*index) + 1 < num && argv[(*index) + 1] != nullptr) {
      ++(*index);
      return to_sv(argv[*index]);
    }
    warn_missing(std::string("--") + std::string(optname));
    return {};
  };

  if (name == "listen") {
    const auto value_req = require_value("listen");
    if (!value_req.empty()) {
      out->listen.assign(value_req);
    }
  } else if (name == "threads") {
    const auto value_req = require_value("threads");
    if (!value_req.empty()) {
      if (auto parsed = parse_int64_sv(value_req)) {
        out->threads = *parsed;
      } else {
        warn_invalid_threads(value_req);
      }
    }
  } else {
    warn_unknown(std::string("--") + std::string(name));
  }
}

inline void handle_short_option(size_t *index, std::span<char *> argv,
                                size_t num, std::string_view token,
                                ParsedArgs *out) {
  const char short_opt = token[1];
  const std::string_view rest_sv = token.substr(2);

  auto require_value_short = [&](char opt) -> std::string_view {
    if (!rest_sv.empty()) {
      return rest_sv;
    }
    if ((*index) + 1 < num && argv[(*index) + 1] != nullptr) {
      ++(*index);
      return to_sv(argv[*index]);
    }
    warn_missing(std::string("-") + std::string(1, opt));
    return {};
  };

  if (short_opt == 'l') {
    const auto value_req = require_value_short('l');
    if (!value_req.empty()) {
      out->listen.assign(value_req);
    }
  } else if (short_opt == 't') {
    const auto value_req = require_value_short('t');
    if (!value_req.empty()) {
      if (auto parsed = parse_int64_sv(value_req)) {
        out->threads = *parsed;
      } else {
        std::cerr << "lemondb: warning: invalid value for -t " << value_req
                  << '\n';
      }
    }
  } else {
    warn_unknown(token);
  }
}
}  // namespace

auto parseArgs(std::span<char *> argv, int argc) -> ParsedArgs {
  ParsedArgs out{};
  const auto num = static_cast<size_t>(argc);

  for (size_t ind = 1; ind < num; ++ind) {
    const std::string_view tok = to_sv(argv[ind]);
    if (tok.empty()) {
      continue;
    }
    if (tok == "--") {
      break;
    }

    // Long options: --name or --name=value
    if (tok.starts_with("--")) {
      handle_long_option(&ind, argv, num, tok, &out);
      continue;
    }

    // Short options: -lVALUE or -l VALUE, -tN or -t N
    if (tok[0] == '-' && tok.size() >= 2) {
      handle_short_option(&ind, argv, num, tok, &out);
      continue;
    }

    warn_unknown(tok);
  }
  return out;
}
