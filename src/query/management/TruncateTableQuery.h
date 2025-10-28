#ifndef SRC_QUERY_MANAGEMENT_TRUNCATETABLEQUERY_H_
#define SRC_QUERY_MANAGEMENT_TRUNCATETABLEQUERY_H_

#include <string>

#include "../Query.h"

class TruncateTableQuery : public Query {
  static constexpr const char *qname = "TRUNCATE";

public:
  using Query::Query;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // SRC_QUERY_MANAGEMENT_TRUNCATETABLEQUERY_H_
