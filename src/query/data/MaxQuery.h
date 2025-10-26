#ifndef PROJECT_MAXQUERY_H
#define PROJECT_MAXQUERY_H

#include <string>
#include <vector>

#include "../Query.h"
#include "../QueryResult.h"

class MaxQuery : public ComplexQuery {
  static constexpr const char *qname = "MAX";

  std::vector<Table::FieldIndex> fieldId;

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // PROJECT_MAXQUERY_H
