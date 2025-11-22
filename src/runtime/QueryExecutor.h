//
// Query execution utilities
//

#ifndef SRC_RUNTIME_QUERYEXECUTOR_H_
#define SRC_RUNTIME_QUERYEXECUTOR_H_

#include <cstddef>
#include <fstream>
#include <istream>
#include <string>

#include "../query/QueryParser.h"
#include "Runtime.h"

auto extractQueryString(std::istream &input_stream) -> std::string;

void outputQueryResult(size_t queryNum, const QueryResult::Ptr &result);

void executeQueries(std::istream &input_stream, std::ifstream &fin,
                    QueryParser &parser, Runtime &runtime, size_t numThreads);

#endif  // SRC_RUNTIME_QUERYEXECUTOR_H_
