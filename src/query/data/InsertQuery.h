//
// Created by liu on 18-10-25.
//

#ifndef SRC_QUERY_DATA_INSERTQUERY_H_
#define SRC_QUERY_DATA_INSERTQUERY_H_

#include <string>

#include "../Query.h"
#include "../QueryResult.h"

class InsertQuery : public ComplexQuery {
  static constexpr const char *qname = "INSERT";

public:
  using ComplexQuery::ComplexQuery;

  auto execute() -> QueryResult::Ptr override;

  auto toString() -> std::string override;

  [[nodiscard]] auto type() const noexcept -> QueryType override {
    return QueryType::Insert;
  }
};

#endif  // SRC_QUERY_DATA_INSERTQUERY_H_
