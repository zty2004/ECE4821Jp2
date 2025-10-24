#ifndef PROJECT_SELECTQUERY_H
#define PROJECT_SELECTQUERY_H

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

#endif // PROJECT_UPDATEQUERY_H
