#ifndef PROJECT_SUBQUERY_H
#define PROJECT_SUBQUERY_H

#include "../Query.h"
#include "../QueryResult.h"

class SubQuery : public ComplexQuery {
  static constexpr const char *qname = "SUB";

  // Destination field and source fields to subtract
  Table::FieldIndex dstId{};
  std::vector<Table::FieldIndex> srcId{};

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // PROJECT_SUBQUERY_H
