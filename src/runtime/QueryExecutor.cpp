//
// Query execution implementation
//

#include "QueryExecutor.h"

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
#include "../query/QueryResult.h"
#include "../query/management/ListenQuery.h"
#include "../utils/uexception.h"

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

namespace {
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void executeQueries(std::istream &input_stream, std::ifstream &fin,
                    QueryParser &parser, Runtime &runtime, size_t numThreads) {
  size_t counter = 0;
  std::queue<std::string> fileQueue;
  std::vector<Query::Ptr> allQueries;

  const bool isInitialFile = !fin.is_open();

  if (isInitialFile) {
    while (input_stream) {
      try {
        std::string const queryStr = extractQueryString(input_stream);
        Query::Ptr query = parser.parseQuery(queryStr);

        const auto *listenQuery =
            dynamic_cast<const ListenQuery *>(query.get());
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
    while (input_stream) {
      try {
        std::string const queryStr = extractQueryString(input_stream);
        Query::Ptr query = parser.parseQuery(queryStr);

        const auto *listenQuery =
            dynamic_cast<const ListenQuery *>(query.get());
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

  for (auto &query : allQueries) {
    ++counter;

    if (numThreads == 1) {
      auto result = query->execute();
      outputQueryResult(counter, result);
    } else {
      runtime.submitQuery(std::move(query), counter);
    }
  }

  if (numThreads > 1) {
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
