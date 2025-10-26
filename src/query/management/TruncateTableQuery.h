#ifndef PROJECT_TRUNCATETABLEQUERY_H
#define PROJECT_TRUNCATETABLEQUERY_H

#include <string>

#include "../Query.h"

class TruncateTableQuery : public Query {
  static constexpr const char *qname = "TRUNCATE";

public:
  using Query::Query;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // PROJECT_TRUNCATETABLEQUERY_H
