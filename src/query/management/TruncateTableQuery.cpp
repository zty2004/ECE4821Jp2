#include "TruncateTableQuery.h"

#include "../../db/Database.h"

constexpr const char *TruncateTableQuery::qname;

QueryResult::Ptr TruncateTableQuery::execute() {
  using namespace std;
  Database &db = Database::getInstance();
  try {
    auto &table = db[this->targetTable];
    table.clear();
    return make_unique<NullQueryResult>();
  } catch (const TableNameNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "No such table."s);
  } catch (const exception &e) {
    return make_unique<ErrorMsgResult>(qname, e.what());
  }
}

std::string TruncateTableQuery::toString() {
  return "QUERY = TRUNCATE, Table = \"" + this->targetTable + "\"";
}
