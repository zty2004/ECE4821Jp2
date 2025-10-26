#ifndef PROJECT_SWAPQUERY_H
#define PROJECT_SWAPQUERY_H

#include "../Query.h"

class SwapQuery : public ComplexQuery {
  static constexpr const char *qname = "SWAP";

  Table::FieldIndex field1Id;
  Table::FieldIndex field2Id;

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // PROJECT_SWAPQUERY_H
