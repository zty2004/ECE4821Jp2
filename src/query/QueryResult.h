//
// Created by liu on 18-10-25.
//

#ifndef SRC_QUERY_QUERYRESULT_H_
#define SRC_QUERY_QUERYRESULT_H_

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "../utils/formatter.h"

class QueryResult {
public:
  typedef std::unique_ptr<QueryResult> Ptr;

  virtual bool success() = 0;

  virtual bool display() = 0;

  virtual ~QueryResult() = default;

  friend std::ostream &operator<<(std::ostream &os, const QueryResult &table);

protected:
  virtual std::ostream &output(std::ostream &os) const = 0;
};

class FailedQueryResult : public QueryResult {
  const std::string data;

public:
  bool success() override { return false; }

  bool display() override { return false; }
};

class SucceededQueryResult : public QueryResult {
public:
  bool success() override { return true; }
  bool display() override { return true; }
};

class NullQueryResult : public SucceededQueryResult {
public:
protected:
  std::ostream &output(std::ostream &os) const override { return os; }
};

class ErrorMsgResult : public FailedQueryResult {
  std::string msg;

public:
  ErrorMsgResult(const char *qname, const std::string &msg) {
    this->msg = R"(Query "?" failed : ?)"_f % qname % msg;
  }

  ErrorMsgResult(const char *qname, const std::string &table,
                 const std::string &msg) {
    this->msg = R"(Query "?" failed in Table "?" : ?)"_f % qname % table % msg;
  }

protected:
  std::ostream &output(std::ostream &os) const override {
    return os << msg << "\n";
  }
};

class SuccessMsgResult : public SucceededQueryResult {
  std::string msg;

public:
  explicit SuccessMsgResult(const int number) {
    this->msg = R"(ANSWER = ?)"_f % number;
  }

  explicit SuccessMsgResult(const std::vector<int> &results) {
    std::stringstream ss;
    ss << "ANSWER = ( ";
    for (auto result : results) {
      ss << result << " ";
    }
    ss << ")";
    this->msg = ss.str();
  }

  explicit SuccessMsgResult(const char *qname) {
    this->msg = R"(Query "?" success.)"_f % qname;
  }

  explicit SuccessMsgResult(const std::string &msg) { this->msg = msg; }

  SuccessMsgResult(const char *qname, const std::string &msg) {
    this->msg = R"(Query "?" success : ?)"_f % qname % msg;
  }

  SuccessMsgResult(const char *qname, const std::string &table,
                   const std::string &msg) {
    this->msg = R"(Query "?" success in Table "?" : ?)"_f % qname % table % msg;
  }

protected:
  std::ostream &output(std::ostream &os) const override {
    return os << msg << "\n";
  }
};

class RecordCountResult : public SucceededQueryResult {
  const int affectedRows;

public:
  bool display() override { return true; }

  explicit RecordCountResult(int count) : affectedRows(count) {}

protected:
  std::ostream &output(std::ostream &os) const override {
    return os << "Affected ? rows."_f % affectedRows << "\n";
  }
};

#endif // SRC_QUERY_QUERYRESULT_H_
