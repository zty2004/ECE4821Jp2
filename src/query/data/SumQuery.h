#ifndef PROJECT_SUMQUERY_H
#define PROJECT_SUMQUERY_H

#include "../Query.h"

class SumQuery : public ComplexQuery {
  static constexpr const char *qname = "SUM";

  // store field index for the fields to sum
  std::vector<Table::FieldIndex> fieldId;

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // PROJECT_SUMQUERY_H
