//
// Created by zuo on 25-10-25.
//

#include "DuplicateQuery.h"

#include <algorithm>

#include "../../db/Database.h"
#include "../QueryResult.h"

constexpr const char *DuplicateQuery::qname;

QueryResult::Ptr DuplicateQuery::execute() {
  using namespace std;
  if (!this->operands.empty())
    return make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid number of operands (? operands)."_f % operands.size());
  Database &db = Database::getInstance();
  try {
    auto &table = db[this->targetTable];
    auto result = initCondition(table);
    if (result.second) {
    }
  } catch (const TableNameNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "No such table."s);
  } catch (const IllFormedQueryCondition &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const exception &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "Unkonwn error '?'."_f % e.what());
  }
}

std::string DuplicateQuery::toString() {
  return "QUERY = Duplicate " + this->targetTable + "\"";
}
