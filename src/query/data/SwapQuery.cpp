#include "SwapQuery.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "../../db/Database.h"

constexpr const char *SwapQuery::qname;

QueryResult::Ptr SwapQuery::execute() {
  Database &db = Database::getInstance();
  try {
    Table::SizeType counter = 0;
    auto &table = db[this->targetTable];

    // check that exactly 2 operands are provided
    if (this->operands.size() != 2) {
      return std::make_unique<ErrorMsgResult>(
          qname, this->targetTable,
          "Invalid number of operands (? operands)."_f % operands.size());
    }

    field1Id = table.getFieldIndex(this->operands[0]);
    field2Id = table.getFieldIndex(this->operands[1]);
    auto result = initCondition(table);
    if (result.second) {
      // if fields are the same, do nothing
      bool flag = field1Id != field2Id;
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (this->evalCondition(*it)) {
          if (flag)
            std::swap((*it)[field1Id], (*it)[field2Id]);
          ++counter;
        }
      }
    }
    return std::make_unique<RecordCountResult>(counter);
  } catch (const TableNameNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const TableFieldNotFound &e) {
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

std::string SwapQuery::toString() {
  return "QUERY = SWAP FROM " + this->targetTable;
}
