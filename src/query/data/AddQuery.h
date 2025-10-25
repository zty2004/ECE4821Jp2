#ifndef PROJECT_ADDQUERY_H
#define PROJECT_ADDQUERY_H

#include "../Query.h"
#include "../QueryResult.h"

class AddQuery : public ComplexQuery {
  static constexpr const char *qname = "ADD";

  // Destination field and source fields to add
  Table::FieldIndex dstId;
  std::vector<Table::FieldIndex> srcId;

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // PROJECT_ADDQUERY_H
