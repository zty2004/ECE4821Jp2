#include "QueryParser.h"

#include <sstream>
#include <string>
#include <utility>

#include "../utils/uexception.h"
#include "Query.h"
#include "QueryBuilders.h"

QueryParser::QueryParser() : first(nullptr), last(nullptr) {}

auto QueryParser::parseQuery(const std::string &queryString) -> Query::Ptr {
  if (first == nullptr) {
    throw QueryBuilderMatchFailed(queryString);
  }
  auto tokenized = tokenizeQueryString(queryString);
  if (tokenized.token.empty()) {
    throw QueryBuilderMatchFailed("");
  }
  first->clear();
  return first->tryExtractQuery(tokenized);
}

void QueryParser::registerQueryBuilder(QueryBuilder::Ptr &&qBuilder) {
  if (first == nullptr) {
    first = std::move(qBuilder);
    last = first.get();
    last->setNext(FailedQueryBuilder::getDefault());
  } else {
    QueryBuilder *temp = last;
    last = qBuilder.get();
    temp->setNext(std::move(qBuilder));
    last->setNext(FailedQueryBuilder::getDefault());
  }
}

auto
QueryParser::tokenizeQueryString(const std::string &queryString) -> TokenizedQueryString {
  TokenizedQueryString tokenized;
  tokenized.rawQeuryString = queryString;
  std::stringstream stream;
  stream.str(queryString);
  std::string tStr;
  while (stream >> tStr) {
    tokenized.token.push_back(tStr);
  }
  return tokenized;
}
