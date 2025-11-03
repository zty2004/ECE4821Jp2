#ifndef SRC_QUERY_DATA_ADDQUERY_H_
#define SRC_QUERY_DATA_ADDQUERY_H_

#include "../Query.h"
#include "../QueryResult.h"

class AddQuery : public ComplexQuery {
  static constexpr const char *qname = "ADD";

  // Destination field and source fields to add
  Table::FieldIndex dstId{};
  std::vector<Table::FieldIndex> srcId{};

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // SRC_QUERY_DATA_ADDQUERY_H_
