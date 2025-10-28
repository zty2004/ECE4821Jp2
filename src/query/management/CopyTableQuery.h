#ifndef SRC_QUERY_MANAGEMENT_COPYTABLEQUERY_H_
#define SRC_QUERY_MANAGEMENT_COPYTABLEQUERY_H_

#include <string>
#include <utility>

#include "../Query.h"

class CopyTableQuery : public Query {
  static constexpr const char *qname = "COPYTABLE";
  const std::string new_table;

public:
  CopyTableQuery(std::string table, std::string new_table)
      : Query(std::move(table)), new_table(std::move(new_table)) {}

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // SRC_QUERY_MANAGEMENT_COPYTABLEQUERY_H_
