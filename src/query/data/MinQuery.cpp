#include "MinQuery.h"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <memory>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../utils/formatter.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"

auto MinQuery::execute() -> QueryResult::Ptr {
  // check operands
  if (this->operands.empty()) {
    return std::make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid number of operands (? operands)."_f % operands.size());
  }

  try {
    auto &database = Database::getInstance();
    auto &table = database[this->targetTable];

    fieldId.clear();
    fieldId.resize(this->operands.size());
    auto ids_view =
        this->operands |
        std::ranges::views::transform(
            [&table](const std::string &fname) -> Table::FieldIndex {
              return table.getFieldIndex(fname);
            });
    std::ranges::copy(ids_view, fieldId.begin());

    auto condInit = initCondition(table);

    // Compute min per field
    std::vector<int> mins(fieldId.size(), Table::ValueTypeMax);
    std::size_t matched = 0;

    if (condInit.second) {
      for (auto &&obj : table) {
        if (evalCondition(obj)) {
          ++matched;
          // Update mins in-place using ranges transform: pair (fieldId, mins)
          std::ranges::transform(
              fieldId, mins, mins.begin(),
              [&obj](Table::FieldIndex fid, int curMin) -> int {
                int const val = obj[fid];
                return val < curMin ? val : curMin;
              });
        }
      }
    }

    if (matched == 0) {
      return std::make_unique<NullQueryResult>();
    }

    return std::make_unique<SuccessMsgResult>(std::move(mins));
  } catch (const TableNameNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const TableFieldNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const IllFormedQueryCondition &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            "Unkonwn error '?'"_f % e.what());
  }
}

auto MinQuery::toString() -> std::string {
  return "QUERY = MIN " + this->targetTable + "\"";
}
