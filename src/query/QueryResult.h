//
// Created by liu on 18-10-25.
//

#ifndef SRC_QUERY_QUERYRESULT_H_
#define SRC_QUERY_QUERYRESULT_H_

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../utils/formatter.h"

class QueryResult {
public:
  using Ptr = std::unique_ptr<QueryResult>;

  QueryResult() = default;

  // Polymorphic base class - delete copy, default move
  QueryResult(const QueryResult &) = delete;
  auto operator=(const QueryResult &) -> QueryResult & = delete;
  QueryResult(QueryResult &&) = default;
  auto operator=(QueryResult &&) -> QueryResult & = default;

  virtual auto success() -> bool = 0;

  virtual auto display() -> bool = 0;

  virtual ~QueryResult() = default;

  friend auto operator<<(std::ostream &os,
                         const QueryResult &table) -> std::ostream &;

protected:
  virtual auto output(std::ostream &os) const -> std::ostream & = 0;
};

class FailedQueryResult : public QueryResult {
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)
  std::string data;

public:
  auto success() -> bool override { return false; }

  auto display() -> bool override { return false; }
};

class SucceededQueryResult : public QueryResult {
public:
  auto success() -> bool override { return true; }
  auto display() -> bool override { return true; }
};

class NullQueryResult : public SucceededQueryResult {
public:
  using Ptr = std::unique_ptr<NullQueryResult>;

protected:
  auto output(std::ostream &os) const -> std::ostream & override { return os; }
};

class ErrorMsgResult : public FailedQueryResult {
  std::string msg;

public:
  ErrorMsgResult(const char *qname, const std::string &msg)
      : msg(R"(Query "?" failed : ?)"_f % qname % msg) {}

  ErrorMsgResult(const char *qname, const std::string &table,
                 const std::string &msg)
      : msg(R"(Query "?" failed in Table "?" : ?)"_f % qname % table % msg) {}

protected:
  auto output(std::ostream &os) const -> std::ostream & override {
    return os << msg << "\n";
  }
};

class SuccessMsgResult : public SucceededQueryResult {
  std::string msg;

public:
  explicit SuccessMsgResult(const int number)
      : msg(R"(ANSWER = ?)"_f % number) {}

  explicit SuccessMsgResult(const std::vector<int> &results) {
    std::stringstream stream;
    stream << "ANSWER = ( ";
    for (auto result : results) {
      stream << result << " ";
    }
    stream << ")";
    this->msg = stream.str();
  }

  explicit SuccessMsgResult(const char *qname)
      : msg(R"(Query "?" success.)"_f % qname) {}

  explicit SuccessMsgResult(std::string msg) : msg(std::move(msg)) {}

  SuccessMsgResult(const char *qname, const std::string &msg)
      : msg(R"(Query "?" success : ?)"_f % qname % msg) {}

  SuccessMsgResult(const char *qname, const std::string &table,
                   const std::string &msg)
      : msg(R"(Query "?" success in Table "?" : ?)"_f % qname % table % msg) {}

protected:
  auto output(std::ostream &os) const -> std::ostream & override {
    return os << msg << "\n";
  }
};

class RecordCountResult : public SucceededQueryResult {
  int affectedRows;

public:
  explicit RecordCountResult(int count) : affectedRows(count) {}

protected:
  auto output(std::ostream &os) const -> std::ostream & override {
    return os << "Affected ? rows."_f % affectedRows << "\n";
  }
};

#endif  // SRC_QUERY_QUERYRESULT_H_
