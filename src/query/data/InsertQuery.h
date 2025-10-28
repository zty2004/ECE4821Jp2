//
// Created by liu on 18-10-25.
//

#ifndef SRC_QUERY_DATA_INSERTQUERY_H_
#define SRC_QUERY_DATA_INSERTQUERY_H_

#include <string>

#include "../Query.h"

class InsertQuery : public ComplexQuery {
  static constexpr const char *qname = "INSERT";

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // SRC_QUERY_DATA_INSERTQUERY_H_
