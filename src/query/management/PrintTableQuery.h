//
// Created by liu on 18-10-25.
//

#ifndef SRC_QUERY_MANAGEMENT_PRINTTABLEQUERY_H_
#define SRC_QUERY_MANAGEMENT_PRINTTABLEQUERY_H_

#include <string>

#include "../Query.h"

class PrintTableQuery : public Query {
  static constexpr const char *qname = "SHOWTABLE";

public:
  using Query::Query;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // SRC_QUERY_MANAGEMENT_PRINTTABLEQUERY_H_
