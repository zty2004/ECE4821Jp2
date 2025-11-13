#ifndef SRC_QUERY_DATA_DELETEQUERY_H_
#define SRC_QUERY_DATA_DELETEQUERY_H_

#include <string>

#include "../Query.h"
#include "../QueryResult.h"

class DeleteQuery : public ComplexQuery {
  static constexpr const char *qname = "DELETE";

public:
  using ComplexQuery::ComplexQuery;

  auto execute() -> QueryResult::Ptr override;

  auto toString() -> std::string override;

  [[nodiscard]] auto type() const noexcept -> QueryType override {
    return QueryType::Delete;
  }
};

#endif  // SRC_QUERY_DATA_DELETEQUERY_H_
