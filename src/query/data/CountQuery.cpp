#include "CountQuery.h"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>

#include "../../db/Database.h"
#include "../../utils/formatter.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"

auto CountQuery::execute() -> QueryResult::Ptr {
  Database &database = Database::getInstance();
  try {
    int64_t counter = 0;
    auto &table = database[this->targetTable];
    auto result = initCondition(table);
    if (result.second) {
      // iterate through all datums and count the num satisfying conditions
      counter = std::count_if(
          table.begin(), table.end(),
          [this](const auto &obj) -> bool { return this->evalCondition(obj); });
    }
    return std::make_unique<SuccessMsgResult>(counter);
  } catch (const TableNameNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const IllFormedQueryCondition &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const std::invalid_argument &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            "Unknown error '?'"_f % e.what());
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            "Unknown error '?'"_f % e.what());
  }
}

auto CountQuery::toString() -> std::string {
  return "QUERY = COUNT FROM " + this->targetTable + "\"";
}
