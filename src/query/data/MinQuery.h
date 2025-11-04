#ifndef SRC_QUERY_DATA_MINQUERY_H_
#define SRC_QUERY_DATA_MINQUERY_H_

#include "../Query.h"
#include "../QueryResult.h"

class MinQuery : public ComplexQuery {
  static constexpr const char *qname = "MIN";

  std::vector<Table::FieldIndex> fieldId;

public:
  using ComplexQuery::ComplexQuery;

  auto execute() -> QueryResult::Ptr override;

  auto toString() -> std::string override;
};

#endif // SRC_QUERY_DATA_MINQUERY_H_
