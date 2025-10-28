#ifndef SRC_QUERY_DATA_SELECTQUERY_H_
#define SRC_QUERY_DATA_SELECTQUERY_H_

#include <string>
#include <vector>

#include "../Query.h"

class SelectQuery : public ComplexQuery {
  static constexpr const char *qname = "SELECT";

  // record the index of all target field
  std::vector<Table::FieldIndex> fieldId;

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // SRC_QUERY_DATA_SELECTQUERY_H_
