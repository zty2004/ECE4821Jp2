#ifndef SRC_QUERY_QUERYPARSER_H_
#define SRC_QUERY_QUERYPARSER_H_

#include <memory>
#include <string>
#include <vector>

#include "Query.h"

struct TokenizedQueryString {
  std::vector<std::string> token;
  std::string rawQeuryString;
};

class QueryBuilder {
public:
  using Ptr = std::unique_ptr<QueryBuilder>;

  virtual auto
  tryExtractQuery(const TokenizedQueryString &queryString) -> Query::Ptr = 0;
  virtual void setNext(Ptr &&builder) = 0;
  virtual void clear() = 0;

  QueryBuilder() = default;
  QueryBuilder(const QueryBuilder &) = delete;
  auto operator=(const QueryBuilder &) -> QueryBuilder & = delete;
  QueryBuilder(QueryBuilder &&) = delete;
  auto operator=(QueryBuilder &&) -> QueryBuilder & = delete;
  virtual ~QueryBuilder() = default;
};

class QueryParser {
  QueryBuilder::Ptr first;       // An owning pointer
  QueryBuilder *last = nullptr;  // None owning reference

  static auto
  tokenizeQueryString(const std::string &queryString) -> TokenizedQueryString;

public:
  auto parseQuery(const std::string &queryString) -> Query::Ptr;
  void registerQueryBuilder(QueryBuilder::Ptr &&qBuilder);

  QueryParser();
  QueryParser(const QueryParser &) = delete;
  auto operator=(const QueryParser &) -> QueryParser & = delete;
  QueryParser(QueryParser &&) = delete;
  auto operator=(QueryParser &&) -> QueryParser & = delete;
  ~QueryParser() = default;
};

#endif  // SRC_QUERY_QUERYPARSER_H_
