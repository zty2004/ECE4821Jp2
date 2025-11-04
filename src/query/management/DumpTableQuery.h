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
  std::string fileName;

public:
  DumpTableQuery(std::string table, std::string filename)
      : Query(std::move(table)), fileName(std::move(filename)) {}

  auto execute() -> QueryResult::Ptr override;

  auto toString() -> std::string override;
};

#endif // SRC_QUERY_MANAGEMENT_DUMPTABLEQUERY_H_
