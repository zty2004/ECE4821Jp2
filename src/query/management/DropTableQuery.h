//
// Created by liu on 18-10-25.
//

#ifndef SRC_QUERY_MANAGEMENT_DROPTABLEQUERY_H_
#define SRC_QUERY_MANAGEMENT_DROPTABLEQUERY_H_

#include <string>

#include "../Query.h"

class DropTableQuery : public Query {
  static constexpr const char *qname = "DROP";

public:
  using Query::Query;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // SRC_QUERY_MANAGEMENT_DROPTABLEQUERY_H_
