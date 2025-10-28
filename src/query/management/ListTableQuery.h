//
// Created by liu on 18-10-25.
//

#ifndef SRC_QUERY_MANAGEMENT_LISTTABLEQUERY_H_
#define SRC_QUERY_MANAGEMENT_LISTTABLEQUERY_H_

#include <string>

#include "../Query.h"

class ListTableQuery : public Query {
  static constexpr const char *qname = "LIST";

public:
  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // SRC_QUERY_MANAGEMENT_LISTTABLEQUERY_H_
