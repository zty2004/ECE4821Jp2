#ifndef SRC_QUERY_MANAGEMENT_COPYTABLEQUERY_H_
#define SRC_QUERY_MANAGEMENT_COPYTABLEQUERY_H_

#include <string>
#include <utility>

#include "../Query.h"

class CopyTableQuery : public Query {
  static constexpr const char *qname = "COPYTABLE";
  std::string new_table;

public:
  CopyTableQuery(std::string table, std::string new_table)
      : Query(std::move(table)), new_table(std::move(new_table)) {}

  auto execute() -> QueryResult::Ptr override;

  auto toString() -> std::string override;

  [[nodiscard]] auto newTable() const -> const std::string & override {
    return new_table;
  }

  [[nodiscard]] auto type() const noexcept -> QueryType override {
    return QueryType::CopyTable;
  }
};

#endif // SRC_QUERY_MANAGEMENT_COPYTABLEQUERY_H_
