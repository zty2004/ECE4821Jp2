//
// Query execution implementation
//

#include "QueryExecutor.h"

#include <cstddef>
#include <cstdio>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "../query/Query.h"
#include "../query/QueryParser.h"
#include "../query/QueryResult.h"
#include "../query/management/ListenQuery.h"
#include "../utils/FallbackAnalyzer.h"
#include "../utils/uexception.h"
#include "Runtime.h"

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
      std::cout.flush();
      std::cerr << *result;
    }
  } else {
    std::cout.flush();
    std::cerr << "QUERY FAILED:\n\t" << *result;
  }
}

namespace {
struct FileProcessResult {
  std::vector<Query::Ptr> queries;
  std::vector<std::string> listenFiles;
};

auto processFile(const std::string &filename,
                 QueryParser &parser)  // NOLINT(runtime/references)
    -> FileProcessResult {
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

      const auto *listenQuery = dynamic_cast<const ListenQuery *>(query.get());
      if (listenQuery != nullptr) {
        result.listenFiles.push_back(listenQuery->getFileName());
      }
      result.queries.push_back(std::move(query));
    } catch (const std::ios_base::failure &) {
      break;
    } catch (const std::exception &exception_obj) {
      std::cout.flush();
      std::cerr << exception_obj.what() << '\n';
    }
  }
  return result;
}
}  // namespace

// Helper: Collect queries from input stream
namespace {
void collectQueriesFromStream(std::istream &input_stream,
                              QueryParser &parser,  // NOLINT
                              std::vector<Query::Ptr> &allQueries,
                              std::queue<std::string> &fileQueue) {
  while (input_stream) {
    try {
      std::string const queryStr = extractQueryString(input_stream);
      Query::Ptr query = parser.parseQuery(queryStr);

      const auto *listenQuery = dynamic_cast<const ListenQuery *>(query.get());
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
}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void executeQueries(std::istream &input_stream,
                    [[maybe_unused]] std::ifstream &fin,
                    QueryParser &parser,  // NOLINT
                    size_t numThreads) {
  size_t counter = 0;
  std::queue<std::string> fileQueue;
  std::vector<Query::Ptr> allQueries;

  // Collect queries from initial stream
  collectQueriesFromStream(input_stream, parser, allQueries, fileQueue);

  while (!fileQueue.empty()) {
    const std::string filename = fileQueue.front();
    fileQueue.pop();

    auto fileResult = processFile(filename, parser);

    allQueries.insert(allQueries.end(),
                      std::make_move_iterator(fileResult.queries.begin()),
                      std::make_move_iterator(fileResult.queries.end()));

    for (const auto &listenFile : fileResult.listenFiles) {
      fileQueue.push(listenFile);
    }
  }

  // Analyze workload and decide fallback
  size_t effectiveThreads = numThreads;
  if (numThreads > 1) {
    const WorkloadStats stats = analyzeWorkload(allQueries);
    const FallbackThresholds thresholds;  // Use default thresholds

    if (shouldFallback(numThreads, stats, thresholds)) {
      effectiveThreads = 1;
      std::cerr << "Falling back to single-threaded mode (low workload: "
                << stats.queryCount << " queries, " << stats.tableCount
                << " tables)\n";
    }
  }

  // Execute queries based on effective thread count
  if (effectiveThreads == 1) {
    // Single-threaded execution
    for (auto &query : allQueries) {
      ++counter;
      auto result = query->execute();
      outputQueryResult(counter, result);
    }
  } else {
    // Multi-threaded execution - create Runtime with effective threads
    Runtime runtime(effectiveThreads);

    for (auto &query : allQueries) {
      ++counter;
      runtime.submitQuery(std::move(query), counter);
    }

    runtime.startExecution();
    runtime.waitAll();
    auto results = runtime.getResultsInOrder();
    for (size_t i = 0; i < results.size(); ++i) {
      if (results[i] == nullptr) {
        throw QuitException();
      }
      outputQueryResult(i + 1, results[i]);
    }
  }
}
