#ifndef SRC_QUERY_DATA_COUNTQUERY_H_
#define SRC_QUERY_DATA_COUNTQUERY_H_

#include <string>

#include "../Query.h"
#include "../QueryResult.h"

class CountQuery : public ComplexQuery {
  static constexpr const char *qname = "COUNT";

public:
  using ComplexQuery::ComplexQuery;

  auto execute() -> QueryResult::Ptr override;

  auto toString() -> std::string override;
};

#endif // SRC_QUERY_DATA_COUNTQUERY_H_
