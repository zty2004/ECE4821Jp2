#include "TruncateTableQuery.h"

#include <memory>
#include <string>
#include <exception>

#include "../../db/Database.h"
#include "../../query/QueryResult.h"
#include "../../utils/uexception.h"

auto TruncateTableQuery::execute() -> QueryResult::Ptr {
  Database &database = Database::getInstance();
  try {
    auto &table = database[this->targetTable];
    table.clear();
    return std::make_unique<NullQueryResult>();
  } catch (const TableNameNotFound &) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            std::string("No such table."));
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, e.what());
  }
}

auto TruncateTableQuery::toString() -> std::string {
  return "QUERY = TRUNCATE, Table = \"" + this->targetTable + "\"";
}
