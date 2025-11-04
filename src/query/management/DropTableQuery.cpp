//
// Created by liu on 18-10-25.
//

#include "DropTableQuery.h"

#include <memory>
#include <string>

#include "../../db/Database.h"

auto DropTableQuery::execute() -> QueryResult::Ptr {
  Database &database = Database::getInstance();
  try {
    database.dropTable(this->targetTable);
    return std::make_unique<NullQueryResult>();
  } catch (const TableNameNotFound &) {
    return std::make_unique<ErrorMsgResult>(qname, targetTable,
                                            std::string("No such table."));
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, e.what());
  }
}

auto DropTableQuery::toString() -> std::string {
  return "QUERY = DROP, Table = \"" + targetTable + "\"";
}
