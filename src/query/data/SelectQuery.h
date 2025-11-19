#ifndef SRC_QUERY_DATA_SELECTQUERY_H_
#define SRC_QUERY_DATA_SELECTQUERY_H_

#include <string>
#include <vector>

#include "../Query.h"
#include "../QueryResult.h"

class SelectQuery : public ComplexQuery {
  static constexpr const char *qname = "SELECT";

  // record the index of all target field
  std::vector<Table::FieldIndex> fieldId;

public:
  using ComplexQuery::ComplexQuery;

  auto execute() -> QueryResult::Ptr override;

  auto toString() -> std::string override;

  [[nodiscard]] auto type() const noexcept -> QueryType override {
    return QueryType::Select;
  }
};

#endif // SRC_QUERY_DATA_SELECTQUERY_H_
