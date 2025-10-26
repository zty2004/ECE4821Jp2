#include "CopyTableQuery.h"

#include <memory>
#include <string>
#include <utility>

#include "../../db/Database.h"

constexpr const char *CopyTableQuery::qname;

QueryResult::Ptr CopyTableQuery::execute() {
  Database &db = Database::getInstance();

  try {
    const auto &table = db[this->targetTable];

    // create a copy of the table with the new name
    auto newTable = std::make_unique<Table>(this->new_table, table);

    // register the new table
    db.registerTable(std::move(newTable));

    return std::make_unique<NullQueryResult>();
  } catch (const TableNameNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const DuplicatedTableName &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->new_table, e.what());
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, e.what());
  }
}

std::string CopyTableQuery::toString() {
  return "QUERY = COPYTABLE, FROM " + this->targetTable + " TO " +
         this->new_table;
}
