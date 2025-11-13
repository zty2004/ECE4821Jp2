//
// Created by liu on 18-10-25.
//

#ifndef SRC_QUERY_QUERYBUILDERS_H_
#define SRC_QUERY_QUERYBUILDERS_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../db/Table.h"
#include "QueryParser.h"

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define QueryBuilder(name) name##QueryBuilder

#define QueryBuilderClass(name)                                                \
  class QueryBuilder(name) : public QueryBuilder {                             \
    auto tryExtractQuery(const TokenizedQueryString &query)                    \
        ->Query::Ptr override;                                                 \
  }

#define BasicQueryBuilderClass(name)                                           \
  class QueryBuilder(name) : public BasicQueryBuilder {                        \
    auto tryExtractQuery(const TokenizedQueryString &query)                    \
        ->Query::Ptr override;                                                 \
  }

#define ComplexQueryBuilderClass(name)                                         \
  class QueryBuilder(name) : public ComplexQueryBuilder {                      \
    auto tryExtractQuery(const TokenizedQueryString &query)                    \
        ->Query::Ptr override;                                                 \
  }
// NOLINTEND(cppcoreguidelines-macro-usage)

class FailedQueryBuilder : public QueryBuilder {
public:
  static auto getDefault() -> QueryBuilder::Ptr {
    return std::make_unique<FailedQueryBuilder>();
  }

  auto tryExtractQuery(const TokenizedQueryString &query) -> Query::Ptr final {
    throw QueryBuilderMatchFailed(query.rawQeuryString);
  }

  // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
  void setNext(QueryBuilder::Ptr && /* builder */) final {}

  void clear() override {}

  FailedQueryBuilder() = default;
  FailedQueryBuilder(const FailedQueryBuilder &) = delete;
  auto operator=(const FailedQueryBuilder &) -> FailedQueryBuilder & = delete;
  FailedQueryBuilder(FailedQueryBuilder &&) = delete;
  auto operator=(FailedQueryBuilder &&) -> FailedQueryBuilder & = delete;
  ~FailedQueryBuilder() override = default;
};

class BasicQueryBuilder : public QueryBuilder {
protected:
  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)
  QueryBuilder::Ptr nextBuilder;

public:
  void setNext(Ptr &&builder) override { nextBuilder = std::move(builder); }

  auto tryExtractQuery(const TokenizedQueryString &query)
      -> Query::Ptr override {
    return nextBuilder->tryExtractQuery(query);
  }

  BasicQueryBuilder() : nextBuilder(FailedQueryBuilder::getDefault()) {}

  void clear() override { nextBuilder->clear(); }

  BasicQueryBuilder(const BasicQueryBuilder &) = delete;
  auto operator=(const BasicQueryBuilder &) -> BasicQueryBuilder & = delete;
  BasicQueryBuilder(BasicQueryBuilder &&) = delete;
  auto operator=(BasicQueryBuilder &&) -> BasicQueryBuilder & = delete;
  ~BasicQueryBuilder() override = default;
};

class ComplexQueryBuilder : public BasicQueryBuilder {
protected:
  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)
  std::string targetTable;
  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)
  std::vector<std::string> operandToken;
  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)
  std::vector<QueryCondition> conditionToken;

  virtual void parseToken(const TokenizedQueryString &query);

private:
  void parseOperands(std::vector<std::string>::const_iterator *iter,
                     const std::vector<std::string>::const_iterator &end);
  void
  parseWhereConditions(std::vector<std::string>::const_iterator *iter,
                       const std::vector<std::string>::const_iterator &end);

public:
  void clear() override;

  // Used as a debugging function.
  // Prints the parsed information
  auto tryExtractQuery(const TokenizedQueryString &query)
      -> Query::Ptr override;
};

// Transparant builder
// It does not modify or extract anything
// It prints current tokenized string
// Use to examine the queries and tokenizer
BasicQueryBuilderClass(Fake);

// Debug commands / Utils
BasicQueryBuilderClass(Debug);

// Load, dump, truncate and delete table
BasicQueryBuilderClass(ManageTable);

// ComplexQueryBuilderClass(UpdateTable);
// ComplexQueryBuilderClass(Insert);
// ComplexQueryBuilderClass(Delete);

#endif  // SRC_QUERY_QUERYBUILDERS_H_
