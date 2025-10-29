//
// Created by liu on 18-10-25.
//

#include "DropTableQuery.h"

#include <memory>
#include <string>

#include "../../db/Database.h"

constexpr const char *DropTableQuery::qname;

QueryResult::Ptr DropTableQuery::execute() {
  Database &db = Database::getInstance();
  try {
    db.dropTable(this->targetTable);
    return std::make_unique<NullQueryResult>();
  } catch (const TableNameNotFound &) {
    return std::make_unique<ErrorMsgResult>(qname, targetTable,
                                            std::string("No such table."));
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, e.what());
  }
}

std::string DropTableQuery::toString() {
  return "QUERY = DROP, Table = \"" + targetTable + "\"";
}
