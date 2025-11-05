//
// Created by liu on 18-10-21.
//

#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "query/Query.h"
#include "query/QueryBuilders.h"
#include "query/QueryParser.h"
#include "query/QueryResult.h"

namespace {
struct ParsedArgs {
  std::string listen;
  int64_t threads = 0;
};

auto parseArgs(std::span<char *> argv, int argc) -> ParsedArgs {
  ParsedArgs out{};
  const auto num = static_cast<size_t>(argc);
  auto to_sv = [](char *param) -> std::string_view {
    return (param != nullptr) ? std::string_view(param) : std::string_view();
  };

  for (size_t i = 1; i < num; ++i) {
    const std::string_view tok = to_sv(argv[i]);
    if (tok.empty()) {
      continue;
    }
    if (tok == "--") {
      ++i;
      break;
    }

    std::cerr << "lemondb: warning: unknown argument " << tok << '\n';
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

} // anonymous namespace

auto main(int argc, char *argv[]) -> int {
  // Assume only C++ style I/O is used in lemondb
  // Do not use printf/fprintf in <cstdio> with this line
  std::ios_base::sync_with_stdio(false);
  const ParsedArgs parsedArgs =
      parseArgs({argv, static_cast<size_t>(argc)}, argc);

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
  } else if (parsedArgs.threads == 0) {
    // @TODO Auto detect the thread num
    std::cerr << "lemondb: info: auto detect thread num" << '\n';
  } else {
    std::cerr << "lemondb: info: running in " << parsedArgs.threads
              << " threads" << '\n';
  }

  QueryParser parser;

  parser.registerQueryBuilder(std::make_unique<QueryBuilder(Debug)>());
  parser.registerQueryBuilder(std::make_unique<QueryBuilder(ManageTable)>());
  parser.registerQueryBuilder(std::make_unique<QueryBuilder(Complex)>());

  size_t counter = 0;

  while (input_stream) {
    try {
      // A very standard REPL
      // REPL: Read-Evaluate-Print-Loop
      std::string const queryStr = extractQueryString(input_stream);
      Query::Ptr query = parser.parseQuery(queryStr);
      QueryResult::Ptr result = query->execute();
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
    } catch (const std::ios_base::failure &) {
      // End of input
      break;
    } catch (const std::exception &exception_obj) {
      std::cout.flush();
      std::cerr << exception_obj.what() << '\n';
    }
  }

  return 0;
}
