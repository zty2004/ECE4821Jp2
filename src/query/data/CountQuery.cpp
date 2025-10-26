#include "CountQuery.h"

#include "../../db/Database.h"

constexpr const char *CountQuery::qname;

QueryResult::Ptr CountQuery::execute() {
  using namespace std;
  Database &db = Database::getInstance();
  try {
    int counter = 0;
    auto &table = db[this->targetTable];
    auto result = initCondition(table);
    if (result.second) {
      // iterate through all datums and count the num satisfying conditions
      for (auto it = table.begin(); it != table.end(); ++it)
        if (this->evalCondition(*it))
          ++counter;
    }
    return make_unique<SuccessMsgResult>(counter);
  } catch (const TableNameNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const IllFormedQueryCondition &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const invalid_argument &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "Unknown error '?'"_f % e.what());
  } catch (const exception &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "Unknown error '?'."_f % e.what());
  }
}

std::string CountQuery::toString() {
  return "QUERY = COUNT FROM " + this->targetTable + "\"";
}
