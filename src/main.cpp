//
// Created by liu on 18-10-21.
//

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <thread>  // NOLINT(build/c++11)

#include "query/QueryBuilders.h"
#include "query/QueryParser.h"
#include "runtime/QueryExecutor.h"
#include "runtime/Runtime.h"
#include "utils/ArgParser.h"
#include "utils/uexception.h"

namespace {
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

  // In production mode, listen argument must be defined
  if (parsedArgs.listen.empty()) {
    std::cerr << "lemondb: error: --listen argument not found, not allowed in "
                 "production mode"
              << '\n';
    exit(-1);
  }

  if (parsedArgs.threads < 0) {
    std::cerr << "lemondb: error: threads num can not be negative value "
              << parsedArgs.threads << '\n';
    exit(-1);
  }

  // Determine thread count (default to 1 for single-threaded mode)
  size_t numThreads = 0;
  if (parsedArgs.threads > 0) {
    // numThreads = static_cast<size_t>(parsedArgs.threads);
    numThreads = 1;
  } else {
    // Auto-detect available hardware threads (threads == 0)
    // numThreads = std::thread::hardware_concurrency();
    numThreads = 1;
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
