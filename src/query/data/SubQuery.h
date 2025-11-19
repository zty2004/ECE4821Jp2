#ifndef SRC_QUERY_DATA_SUBQUERY_H_
#define SRC_QUERY_DATA_SUBQUERY_H_

#include <string>
#include <vector>

#include "../../db/Table.h"
#include "../Query.h"
#include "../QueryResult.h"

class SubQuery : public ComplexQuery {
  static constexpr const char *qname = "SUB";

  // Destination field and source fields to subtract
  Table::FieldIndex dstId{};
  std::vector<Table::FieldIndex> srcId;

public:
  using ComplexQuery::ComplexQuery;

  auto execute() -> QueryResult::Ptr override;

  auto toString() -> std::string override;

  [[nodiscard]] auto type() const noexcept -> QueryType override {
    return QueryType::Sub;
  }
};

#endif // SRC_QUERY_DATA_SUBQUERY_H_
