//
// Created by liu on 18-10-25.
//

#ifndef SRC_QUERY_MANAGEMENT_LOADTABLEQUERY_H_
#define SRC_QUERY_MANAGEMENT_LOADTABLEQUERY_H_

#include <string>
#include <utility>

#include "../Query.h"

class LoadTableQuery : public Query {
  static constexpr const char *qname = "LOAD";
  const std::string fileName;

public:
  explicit LoadTableQuery(std::string table, std::string fileName)
      : Query(std::move(table)), fileName(std::move(fileName)) {}

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // SRC_QUERY_MANAGEMENT_LOADTABLEQUERY_H_
