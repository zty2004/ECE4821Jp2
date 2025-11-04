#include "MaxQuery.h"

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

auto MaxQuery::execute() -> QueryResult::Ptr {
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

    // Compute max per field
    std::vector<int> maxs(fieldId.size(), Table::ValueTypeMin);
    std::size_t matched = 0;

    if (condInit.second) {
      for (auto &&obj : table) {
        if (evalCondition(obj)) {
          ++matched;
          std::ranges::transform(
              fieldId, maxs, maxs.begin(),
              [&obj](Table::FieldIndex fid, int curMax) -> int {
                int const val = obj[fid];
                return val > curMax ? val : curMax;
              });
        }
      }
    }

    if (matched == 0) {
      return std::make_unique<NullQueryResult>();
    }

    return std::make_unique<SuccessMsgResult>(std::move(maxs));
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

auto MaxQuery::toString() -> std::string {
  return "QUERY = MAX " + this->targetTable + "\"";
}
