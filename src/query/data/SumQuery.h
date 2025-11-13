#ifndef SRC_QUERY_DATA_SUMQUERY_H_
#define SRC_QUERY_DATA_SUMQUERY_H_

#include <string>
#include <vector>

#include "../Query.h"
#include "../QueryResult.h"

class SumQuery : public ComplexQuery {
  static constexpr const char *qname = "SUM";

  // store field index for the fields to sum
  std::vector<Table::FieldIndex> fieldId;

public:
  using ComplexQuery::ComplexQuery;

  auto execute() -> QueryResult::Ptr override;

  auto toString() -> std::string override;

  [[nodiscard]] auto type() const noexcept -> QueryType override {
    return QueryType::Sum;
  }
};

#endif  // SRC_QUERY_DATA_SUMQUERY_H_
