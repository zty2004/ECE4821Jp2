#include "CopyTableQuery.h"

#include <exception>
#include <memory>
#include <string>
#include <utility>

#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../query/QueryResult.h"
#include "../../utils/uexception.h"

auto CopyTableQuery::execute() -> QueryResult::Ptr {
  Database &database = Database::getInstance();

  try {
    const auto &table = database[this->targetTable];

    // create a copy of the table with the new name
    auto newTable = std::make_unique<Table>(this->new_table, table);

    // register the new table
    database.registerTable(std::move(newTable));

    return std::make_unique<NullQueryResult>();
  } catch (const TableNameNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const DuplicatedTableName &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->new_table, e.what());
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, e.what());
  }
}

auto CopyTableQuery::toString() -> std::string {
  return "QUERY = COPYTABLE, FROM " + this->targetTable + " TO " +
         this->new_table;
}
