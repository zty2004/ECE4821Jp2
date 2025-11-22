//
// Argument parser for lemondb
//

#ifndef SRC_UTILS_ARGPARSER_H_
#define SRC_UTILS_ARGPARSER_H_

#include <cstdint>
#include <span>
#include <string>

struct ParsedArgs {
  std::string listen;
  int64_t threads = 0;
};

auto parseArgs(std::span<char *> argv, int argc) -> ParsedArgs;

#endif  // SRC_UTILS_ARGPARSER_H_
