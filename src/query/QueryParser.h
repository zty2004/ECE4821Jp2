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

// NOLINTBEGIN(cppcoreguidelines-special-member-functions,
//             modernize-use-using,
//             modernize-use-trailing-return-type)
class QueryBuilder {
public:
  typedef std::unique_ptr<QueryBuilder> Ptr;

  virtual Query::Ptr
  tryExtractQuery(const TokenizedQueryString &queryString) = 0;
  virtual void setNext(Ptr &&builder) = 0;
  virtual void clear() = 0;

  virtual ~QueryBuilder() = default;
};
// NOLINTEND(cppcoreguidelines-special-member-functions,
//           modernize-use-using,
//           modernize-use-trailing-return-type)

// NOLINTBEGIN(cppcoreguidelines-special-member-functions,
//             modernize-use-trailing-return-type)
class QueryParser {
  QueryBuilder::Ptr first;      // An owning pointer
  QueryBuilder *last{nullptr};  // None owning reference

  static TokenizedQueryString
  tokenizeQueryString(const std::string &queryString);

public:
  Query::Ptr parseQuery(const std::string &queryString);
  void registerQueryBuilder(QueryBuilder::Ptr &&qBuilder);

  QueryParser();
  ~QueryParser() = default;
};
// NOLINTEND(cppcoreguidelines-special-member-functions,
//           modernize-use-trailing-return-type)

#endif  // SRC_QUERY_QUERYPARSER_H_
