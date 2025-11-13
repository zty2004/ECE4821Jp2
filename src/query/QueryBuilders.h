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

#define QueryBuilder(name) name##QueryBuilder

#define QueryBuilderClass(name)                                                \
  class QueryBuilder(name) : public QueryBuilder {                             \
    Query::Ptr tryExtractQuery(const TokenizedQueryString &query) override;    \
  }

#define BasicQueryBuilderClass(name)                                           \
  class QueryBuilder(name) : public BasicQueryBuilder {                        \
    Query::Ptr tryExtractQuery(const TokenizedQueryString &query) override;    \
  }

#define ComplexQueryBuilderClass(name)                                         \
  class QueryBuilder(name) : public ComplexQueryBuilder {                      \
    Query::Ptr tryExtractQuery(const TokenizedQueryString &query) override;    \
  }

class FailedQueryBuilder : public QueryBuilder {
public:
  static QueryBuilder::Ptr getDefault() {
    return std::make_unique<FailedQueryBuilder>();
  }

  Query::Ptr tryExtractQuery(const TokenizedQueryString &q) final {
    throw QueryBuilderMatchFailed(q.rawQeuryString);
  }

  void setNext(QueryBuilder::Ptr &&) final {}

  void clear() override {}

  ~FailedQueryBuilder() override = default;
};

class BasicQueryBuilder : public QueryBuilder {
protected:
  QueryBuilder::Ptr nextBuilder;

public:
  void setNext(Ptr &&builder) override { nextBuilder = std::move(builder); }

  Query::Ptr tryExtractQuery(const TokenizedQueryString &query) override {
    return nextBuilder->tryExtractQuery(query);
  }

  BasicQueryBuilder() : nextBuilder(FailedQueryBuilder::getDefault()) {}

  void clear() override { nextBuilder->clear(); }

  ~BasicQueryBuilder() override = default;
};

class ComplexQueryBuilder : public BasicQueryBuilder {
protected:
  std::string targetTable;
  std::vector<std::string> operandToken;
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

public:
  // Used as a debugging function.
  // Prints the parsed information
  Query::Ptr tryExtractQuery(const TokenizedQueryString &query) override;
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
