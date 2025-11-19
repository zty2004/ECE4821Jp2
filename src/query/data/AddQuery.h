#ifndef SRC_QUERY_DATA_ADDQUERY_H_
#define SRC_QUERY_DATA_ADDQUERY_H_

#include <string>
#include <vector>

#include "../../db/Table.h"
#include "../Query.h"
#include "../QueryResult.h"

class AddQuery : public ComplexQuery {
  static constexpr const char *qname = "ADD";

  // Destination field and source fields to add
  Table::FieldIndex dstId{};
  std::vector<Table::FieldIndex> srcId;

public:
  using ComplexQuery::ComplexQuery;

  auto execute() -> QueryResult::Ptr override;

  auto toString() -> std::string override;

  [[nodiscard]] auto type() const noexcept -> QueryType override {
    return QueryType::Add;
  }
};

#endif // SRC_QUERY_DATA_ADDQUERY_H_
