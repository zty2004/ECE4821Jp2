//
// Created by liu on 18-10-25.
//

#ifndef SRC_QUERY_MANAGEMENT_DUMPTABLEQUERY_H_
#define SRC_QUERY_MANAGEMENT_DUMPTABLEQUERY_H_

#include <string>
#include <utility>

#include "../Query.h"

class DumpTableQuery : public Query {
  static constexpr const char *qname = "DUMP";
  const std::string fileName;

public:
  DumpTableQuery(std::string table, std::string filename)
      : Query(std::move(table)), fileName(std::move(filename)) {}

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // SRC_QUERY_MANAGEMENT_DUMPTABLEQUERY_H_
