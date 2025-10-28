#ifndef SRC_QUERY_DATA_MAXQUERY_H_
#define SRC_QUERY_DATA_MAXQUERY_H_

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

#endif // SRC_QUERY_DATA_MAXQUERY_H_
