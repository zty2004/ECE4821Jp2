//
// Created by liu on 18-10-21.
//

#include <bits/getopt_ext.h>
#include <getopt.h> // NOLINT(misc-include-cleaner)

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "query/Query.h"
#include "query/QueryBuilders.h"
#include "query/QueryParser.h"
#include "query/QueryResult.h"

struct ParsedArgs {
  std::string listen;
  int64_t threads = 0;
};

auto parseArgs(int argc,
               char *argv[])
    -> ParsedArgs { // NOLINT(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  constexpr int base_ten = 10;
  ParsedArgs args{};
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays,misc-include-cleaner)
  const option longOpts[] = {
      {"listen", required_argument, nullptr,
       'l'}, // NOLINT(misc-include-cleaner)
      {"threads", required_argument, nullptr,
       't'},                               // NOLINT(misc-include-cleaner)
      {nullptr, no_argument, nullptr, 0}}; // NOLINT(misc-include-cleaner)
  const char *shortOpts = "l:t:";
  int opt = 0;
  int longIndex = 0;
  // NOLINTNEXTLINE(misc-include-cleaner,cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  while ((opt = getopt_long(argc, argv, shortOpts, longOpts, &longIndex)) !=
         -1) {
    if (opt == 'l') {
      args.listen = optarg; // NOLINT(misc-include-cleaner)
    } else if (opt == 't') {
      args.threads = std::strtoll(optarg, nullptr,
                                  base_ten); // NOLINT(misc-include-cleaner)
    } else {
      std::cerr
          << "lemondb: warning: unknown argument "
          // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
          << longOpts[longIndex].name << '\n';
    }
  }
  return args;
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

auto main(int argc, char *argv[]) -> int {
  // Assume only C++ style I/O is used in lemondb
  // Do not use printf/fprintf in <cstdio> with this line
  std::ios_base::sync_with_stdio(false);
  const ParsedArgs parsedArgs = parseArgs(argc, argv);

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
