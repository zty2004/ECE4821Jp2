#ifndef SRC_QUERY_DATA_SWAPQUERY_H_
#define SRC_QUERY_DATA_SWAPQUERY_H_

#include <string>

#include "../../db/Table.h"
#include "../Query.h"
#include "../QueryResult.h"

class SwapQuery : public ComplexQuery {
  static constexpr const char *qname = "SWAP";

  Table::FieldIndex field1Id{};
  Table::FieldIndex field2Id{};

public:
  using ComplexQuery::ComplexQuery;

  auto execute() -> QueryResult::Ptr override;

  auto toString() -> std::string override;
};

#endif // SRC_QUERY_DATA_SWAPQUERY_H_
