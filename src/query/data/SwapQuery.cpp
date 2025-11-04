#include "SwapQuery.h"

#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../utils/formatter.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"

auto SwapQuery::execute() -> QueryResult::Ptr {
  Database &database = Database::getInstance();
  try {
    Table::SizeType counter = 0;
    auto &table = database[this->targetTable];

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
      // if fields are the same, only count as affected number
      bool const flag = field1Id != field2Id;
      for (auto &&obj : table) {
        if (this->evalCondition(obj)) {
          if (flag) {
            std::swap(obj[field1Id], obj[field2Id]);
          }
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

auto SwapQuery::toString() -> std::string {
  return "QUERY = SWAP FROM " + this->targetTable;
}
