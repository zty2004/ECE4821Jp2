#ifndef SRC_QUERY_DATA_COUNTQUERY_H_
#define SRC_QUERY_DATA_COUNTQUERY_H_

#include <string>

#include "../Query.h"
#include "../QueryResult.h"
class CountQuery : public ComplexQuery {
  static constexpr const char *qname = "COUNT";

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // SRC_QUERY_DATA_COUNTQUERY_H_
