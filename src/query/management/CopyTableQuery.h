#ifndef PROJECT_COPYTABLEQUERY_H
#define PROJECT_COPYTABLEQUERY_H

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

#endif // PROJECT_COPYTABLEQUERY_H
