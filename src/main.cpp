//
// Created by liu on 18-10-21.
//

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <system_error>

#include <charconv>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <queue>
#include <span>
#include <string>
#include <string_view>
#include <thread>  // NOLINT(build/c++11)
#include <utility>
#include <vector>

#include "query/Query.h"
#include "query/QueryBuilders.h"
#include "query/QueryParser.h"
#include "query/QueryResult.h"
#include "query/management/ListenQuery.h"
#include "runtime/Runtime.h"

namespace {
// constants and small helpers for argument parsing
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

struct ParsedArgs {
  std::string listen;
  int64_t threads = 0;
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

void outputQueryResult(size_t queryNum, const QueryResult::Ptr &result) {
  std::cout << queryNum << "\n";
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

// Process a single file and collect LISTEN queries
struct FileProcessResult {
  std::vector<Query::Ptr> queries;
  std::vector<std::string> listenFiles;
};

auto processFile(const std::string &filename,
                 QueryParser &parser) -> FileProcessResult {
  FileProcessResult result;

  std::ifstream fin(filename);
  if (!fin.is_open()) {
    std::cerr << "Error: could not open " << filename << "\n";
    return result;
  }

  std::istream &input_stream = fin;

  while (input_stream) {
    try {
      std::string const queryStr = extractQueryString(input_stream);
      Query::Ptr query = parser.parseQuery(queryStr);

      // Check if this is a LISTEN query
      auto *listenQuery = dynamic_cast<ListenQuery *>(query.get());
      if (listenQuery != nullptr) {
        // Enqueue the listen file for later processing
        result.listenFiles.push_back(listenQuery->getFileName());
      }

      // Store all queries (including LISTEN)
      result.queries.push_back(std::move(query));
    } catch (const std::ios_base::failure &) {
      break;  // End of file
    } catch (const std::exception &exception_obj) {
      std::cout.flush();
      std::cerr << exception_obj.what() << '\n';
    }
  }
  return result;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void executeQueries(std::istream &input_stream, std::ifstream &fin,
                    QueryParser &parser, Runtime &runtime, size_t numThreads) {
  size_t counter = 0;
  std::queue<std::string> fileQueue;
  std::vector<Query::Ptr> allQueries;

  // BFS-style file traversal
  // Process initial input (either from file or stdin)
  const bool isInitialFile = !fin.is_open();

  if (isInitialFile) {
    // Read from stdin - process inline
    while (input_stream) {
      try {
        std::string const queryStr = extractQueryString(input_stream);
        Query::Ptr query = parser.parseQuery(queryStr);

        auto *listenQuery = dynamic_cast<ListenQuery *>(query.get());
        if (listenQuery != nullptr) {
          fileQueue.push(listenQuery->getFileName());
        }

        allQueries.push_back(std::move(query));
      } catch (const std::ios_base::failure &) {
        break;
      } catch (const std::exception &exception_obj) {
        std::cout.flush();
        std::cerr << exception_obj.what() << '\n';
      }
    }
  } else {
    // Read from file - use BFS
    // Get initial filename from fin
    // Note: We need to extract filename, but it's not stored in ifstream
    // So we'll process the current stream first
    while (input_stream) {
      try {
        std::string const queryStr = extractQueryString(input_stream);
        Query::Ptr query = parser.parseQuery(queryStr);

        auto *listenQuery = dynamic_cast<ListenQuery *>(query.get());
        if (listenQuery != nullptr) {
          fileQueue.push(listenQuery->getFileName());
        }

        allQueries.push_back(std::move(query));
      } catch (const std::ios_base::failure &) {
        break;
      } catch (const std::exception &exception_obj) {
        std::cout.flush();
        std::cerr << exception_obj.what() << '\n';
      }
    }
  }

  // Process queued files in BFS order
  while (!fileQueue.empty()) {
    const std::string filename = fileQueue.front();
    fileQueue.pop();

    auto fileResult = processFile(filename, parser);

    // Append queries from this file
    allQueries.insert(allQueries.end(),
                      std::make_move_iterator(fileResult.queries.begin()),
                      std::make_move_iterator(fileResult.queries.end()));

    // Enqueue any LISTEN files found
    for (const auto &listenFile : fileResult.listenFiles) {
      fileQueue.push(listenFile);
    }
  }

  // Execute all queries in order
  for (auto &query : allQueries) {
    ++counter;

    if (numThreads == 1) {
      // Single-threaded: execute and output immediately
      auto result = query->execute();
      outputQueryResult(counter, result);
    } else {
      // Multi-threaded: submit to runtime
      runtime.submitQuery(std::move(query), counter);
    }
  }

  // Multi-threaded mode: start execution and wait for results
  if (numThreads > 1) {
    runtime.startExecution();
    runtime.waitAll();
    auto results = runtime.getResultsInOrder();
    for (size_t i = 0; i < results.size(); ++i) {
      if (results[i] == nullptr) {
        // QUIT command encountered - exit cleanly without output
        throw QuitException();
      }
      outputQueryResult(i + 1, results[i]);
    }
  }
}

// NOLINTNEXTLINE(readability/fn_size)
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

  if (parsedArgs.threads < 0) {
    std::cerr << "lemondb: error: threads num can not be negative value "
              << parsedArgs.threads << '\n';
    exit(-1);
  }

  // Determine thread count (default to 1 for single-threaded mode)
  size_t numThreads = 0;
  if (parsedArgs.threads > 0) {
    numThreads = static_cast<size_t>(parsedArgs.threads);
  } else {
    // Auto-detect available hardware threads (threads == 0)
    numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) {
      // hardware_concurrency() may return 0 if unable to detect
      std::cerr << "lemondb: warning: unable to detect hardware threads, "
                   "defaulting to 1"
                << '\n';
      numThreads = 1;
    }
  }

  if (numThreads == 1) {
    std::cerr << "lemondb: info: running in single-threaded mode" << '\n';
  } else {
    std::cerr << "lemondb: info: running in " << numThreads << " threads";
    if (parsedArgs.threads == 0) {
      std::cerr << " (auto-detected)";
    }
    std::cerr << '\n';
  }

  QueryParser parser;
  parser.registerQueryBuilder(std::make_unique<QueryBuilder(Debug)>());
  parser.registerQueryBuilder(std::make_unique<QueryBuilder(ManageTable)>());
  parser.registerQueryBuilder(std::make_unique<QueryBuilder(Complex)>());

  Runtime runtime(numThreads);

  executeQueries(input_stream, fin, parser, runtime, numThreads);

  return 0;
}
}  // anonymous namespace

auto main(int argc, char *argv[]) -> int {
  try {
    return run({argv, static_cast<size_t>(argc)}, argc);
  } catch (const QuitException &) {
    // Normal exit via QUIT command - allow clean shutdown
    return EXIT_SUCCESS;
  } catch (const std::exception &e) {
    std::cerr << "lemondb: fatal: " << e.what() << '\n';
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "lemondb: fatal: unknown exception" << '\n';
    return EXIT_FAILURE;
  }
}
