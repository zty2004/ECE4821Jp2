//
// Created by liu on 18-10-25.
//

#include "ListTableQuery.h"

#include <memory>
#include <string>

#include "../../db/Database.h"
#include "../../query/QueryResult.h"

auto ListTableQuery::execute() -> QueryResult::Ptr {
  Database &database = Database::getInstance();
  database.printAllTable();
  return std::make_unique<SuccessMsgResult>(qname);
}

auto ListTableQuery::toString() -> std::string { return "QUERY = LIST"; }
