//
// Created by liu on 18-10-25.
//

#ifndef SRC_QUERY_MANAGEMENT_QUITQUERY_H_
#define SRC_QUERY_MANAGEMENT_QUITQUERY_H_

#include <string>

#include "../Query.h"

class QuitQuery : public Query {
  static constexpr const char *qname = "QUIT";

public:
  QuitQuery() = default;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // SRC_QUERY_MANAGEMENT_QUITQUERY_H_
