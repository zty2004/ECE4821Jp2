//
// Created by liu on 18-10-21.
//

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#ifdef ENABLE_RUNTIME
#include <thread>
#endif

#include "query/Query.h"
#include "query/QueryBuilders.h"
#include "query/QueryParser.h"
#include "query/QueryResult.h"

#ifdef ENABLE_RUNTIME
#include "runtime/Runtime.h"
#endif

namespace {
// constants and small helpers for argument parsing
#ifdef ENABLE_RUNTIME
constexpr int dec = 10;
#endif

inline auto to_sv(char *param) -> std::string_view {
  return (param != nullptr) ? std::string_view(param) : std::string_view();
}

inline void warn_unknown(std::string_view token) {
  std::cerr << "lemondb: warning: unknown argument " << token << '\n';
}

inline void warn_missing(std::string_view opt) {
  std::cerr << "lemondb: warning: missing value for " << opt << '\n';
}

#ifdef ENABLE_RUNTIME
inline void warn_invalid_threads(std::string_view value) {
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
#endif

struct ParsedArgs {
  std::string listen;
#ifdef ENABLE_RUNTIME
  int64_t threads = 0;
#endif
};

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
#ifdef ENABLE_RUNTIME
  } else if (name == "threads") {
    const auto value_req = require_value("threads");
    if (!value_req.empty()) {
      if (auto parsed = parse_int64_sv(value_req)) {
        out->threads = *parsed;
      } else {
        warn_invalid_threads(value_req);
      }
    }
#endif
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
#ifdef ENABLE_RUNTIME
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
#endif
  } else {
    warn_unknown(token);
  }
}

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

auto extractQueryString(std::istream &input_stream) -> std::string {
  std::string buf;
  while (true) {
    int const char_read = input_stream.get();
    if (char_read == ';') {
      return buf;
    }
    if (char_read == EOF) {
      throw std::ios_base::failure("End of input");
    }
    buf.push_back(static_cast<char>(char_read));
  }
}

// NOLINTNEXTLINE(readability-function-size)
auto run(std::span<char *> argv, int argc) -> int {
  // Assume only C++ style I/O is used in lemondb
  // Do not use printf/fprintf in <cstdio> with this line
  std::ios_base::sync_with_stdio(false);
  const ParsedArgs parsedArgs = parseArgs(argv, argc);

  std::ifstream fin;
  if (!parsedArgs.listen.empty()) {
    fin.open(parsedArgs.listen);
    if (!fin.is_open()) {
      std::cerr << "lemondb: error: " << parsedArgs.listen
                << ": no such file or directory" << '\n';
      exit(-1);
    }
  }
  std::istream input_stream(fin.rdbuf());

#ifdef NDEBUG
  // In production mode, listen argument must be defined
  if (parsedArgs.listen.empty()) {
    std::cerr << "lemondb: error: --listen argument not found, not allowed in "
                 "production mode"
              << '\n';
    exit(-1);
  }
#else
  // In debug mode, use stdin as input if no listen file is found
  if (parsedArgs.listen.empty()) {
    std::cerr << "lemondb: warning: --listen argument not found, use stdin "
                 "instead in debug mode"
              << '\n';
    input_stream.rdbuf(std::cin.rdbuf());
  }
#endif

#ifdef ENABLE_RUNTIME
  if (parsedArgs.threads < 0) {
    std::cerr << "lemondb: error: threads num can not be negative value "
              << parsedArgs.threads << '\n';
    exit(-1);
  }

  // Determine thread count
  size_t numThreads;
  if (parsedArgs.threads == 0) {
    numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0)
      numThreads = 4;
    std::cerr << "lemondb: info: auto detected " << numThreads << " threads"
              << '\n';
  } else {
    numThreads = static_cast<size_t>(parsedArgs.threads);
    std::cerr << "lemondb: info: running in " << numThreads << " threads"
              << '\n';
  }
#endif

  QueryParser parser;

  parser.registerQueryBuilder(std::make_unique<QueryBuilder(Debug)>());
  parser.registerQueryBuilder(std::make_unique<QueryBuilder(ManageTable)>());
  parser.registerQueryBuilder(std::make_unique<QueryBuilder(Complex)>());

#ifdef ENABLE_RUNTIME
  // Use runtime if threads > 1
  std::unique_ptr<Runtime> runtime;
  if (numThreads > 1) {
    runtime = std::make_unique<Runtime>(numThreads);
  }
#endif

  size_t counter = 0;

  while (input_stream) {
    try {
      // A very standard REPL
      // REPL: Read-Evaluate-Print-Loop
      std::string const queryStr = extractQueryString(input_stream);
      Query::Ptr query = parser.parseQuery(queryStr);

      QueryResult::Ptr result;
#ifdef ENABLE_RUNTIME
      if (runtime) {
        // Multi-threaded execution
        runtime->submitQuery(std::move(query), counter);
      } else {
        // Single-threaded execution
        result = query->execute();
        std::cout << ++counter << "\n";
        if (result->success()) {
          if (result->display()) {
            std::cout << *result;
          } else {
#ifndef NDEBUG
            std::cout.flush();
            std::cerr << *result;
#endif
          }
        } else {
          std::cout.flush();
          std::cerr << "QUERY FAILED:\n\t" << *result;
        }
      }
#else
      // Single-threaded execution (ENABLE_RUNTIME not defined)
      result = query->execute();
      std::cout << ++counter << "\n";
      if (result->success()) {
        if (result->display()) {
          std::cout << *result;
        } else {
#ifndef NDEBUG
          std::cout.flush();
          std::cerr << *result;
#endif
        }
      } else {
        std::cout.flush();
        std::cerr << "QUERY FAILED:\n\t" << *result;
      }
#endif
    } catch (const std::ios_base::failure &) {
      // End of input
      break;
    } catch (const std::exception &exception_obj) {
      std::cout.flush();
      std::cerr << exception_obj.what() << '\n';
    }
  }

#ifdef ENABLE_RUNTIME
  // If using runtime, wait for all queries and output results
  if (runtime) {
    runtime->waitAll();
    auto results = runtime->getResultsInOrder();

    for (size_t i = 0; i < results.size(); ++i) {
      std::cout << (i + 1) << "\n";
      const auto &result = results[i];
      if (result->success()) {
        if (result->display()) {
          std::cout << *result;
        } else {
#ifndef NDEBUG
          std::cout.flush();
          std::cerr << *result;
#endif
        }
      } else {
        std::cout.flush();
        std::cerr << "QUERY FAILED:\n\t" << *result;
      }
    }
  }
#endif

  return 0;
}
}  // anonymous namespace

auto main(int argc, char *argv[]) -> int {
  try {
    return run({argv, static_cast<size_t>(argc)}, argc);
  } catch (const std::exception &e) {
    std::cerr << "lemondb: fatal: " << e.what() << '\n';
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "lemondb: fatal: unknown exception" << '\n';
    return EXIT_FAILURE;
  }
}
