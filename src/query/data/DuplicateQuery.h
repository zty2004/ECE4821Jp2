#ifndef SRC_QUERY_DATA_DUPLICATEQUERY_H_
#define SRC_QUERY_DATA_DUPLICATEQUERY_H_

#include <string>

#include "../Query.h"
#include "../QueryResult.h"

class DuplicateQuery : public ComplexQuery {
  static constexpr const char *qname = "DUPLICATE";

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // SRC_QUERY_DATA_DUPLICATEQUERY_H_
