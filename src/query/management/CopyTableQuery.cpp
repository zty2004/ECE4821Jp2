#include "CopyTableQuery.h"

#include "../../db/Database.h"

constexpr const char *CopyTableQuery::qname;

QueryResult::Ptr CopyTableQuery::execute() {
  using namespace std;
  Database &db = Database::getInstance();

  try {
    const auto &table = db[this->targetTable];

    // create a copy of the table with the new name
    auto newTable = make_unique<Table>(this->new_table, table);

    // register the new table
    db.registerTable(move(newTable));

    return make_unique<NullQueryResult>();
  } catch (const TableNameNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const DuplicatedTableName &e) {
    return make_unique<ErrorMsgResult>(qname, this->new_table, e.what());
  } catch (const exception &e) {
    return make_unique<ErrorMsgResult>(qname, e.what());
  }
}

std::string CopyTableQuery::toString() {
  return "QUERY = COPYTABLE, FROM " + this->targetTable + " TO " +
         this->new_table;
}
