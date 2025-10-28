#ifndef SRC_QUERY_DATA_SUMQUERY_H_
#define SRC_QUERY_DATA_SUMQUERY_H_

#include <string>
#include <vector>

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

#endif // SRC_QUERY_DATA_SUMQUERY_H_
