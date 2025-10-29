#include "TruncateTableQuery.h"

#include <memory>
#include <string>

#include "../../db/Database.h"

constexpr const char *TruncateTableQuery::qname;

QueryResult::Ptr TruncateTableQuery::execute() {
  Database &db = Database::getInstance();
  try {
    auto &table = db[this->targetTable];
    table.clear();
    return std::make_unique<NullQueryResult>();
  } catch (const TableNameNotFound &) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            std::string("No such table."));
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, e.what());
  }
}

std::string TruncateTableQuery::toString() {
  return "QUERY = TRUNCATE, Table = \"" + this->targetTable + "\"";
}
