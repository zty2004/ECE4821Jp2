#include "AddQuery.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
#include <memory>
#include <numeric>
#include <ranges>
#include <string>

#include "../../db/Database.h"
#include "../../utils/formatter.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"

auto AddQuery::execute() -> QueryResult::Ptr {
  // Expect at least dst + one src
  if (this->operands.size() < 2) {
    return std::make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid number of operands (? operands)."_f % operands.size());
  }

  try {
    auto &database = Database::getInstance();
    auto &table = database[this->targetTable];

    dstId = table.getFieldIndex(this->operands.back());
    // count the number of sources (minus the last operand)
    const auto srcCount = this->operands.size() - 1;
    srcId.clear();
    srcId.reserve(srcCount);
    auto ids_view =
        this->operands | std::ranges::views::take(srcCount) |
        std::ranges::views::transform(
            [&table](const std::string &fname) -> Table::FieldIndex {
              return table.getFieldIndex(fname);
            });
    std::ranges::copy(ids_view, std::back_inserter(srcId));

    auto condInit = initCondition(table);

    // Iterate and update
    size_t counter = 0;
    if (condInit.second) {
      for (auto &&obj : table) {
        if (evalCondition(obj)) {
          // use int64_t to avoid overflow during accumulation
          int64_t const sum = std::accumulate(
              srcId.begin(), srcId.end(), 0LL,
              [&obj](int64_t acc, Table::FieldIndex idx) -> int64_t {
                return acc + obj[idx];
              });
          obj[dstId] = static_cast<int>(sum);
          ++counter;
        }
      }
    }

    return std::make_unique<RecordCountResult>(static_cast<int>(counter));
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

auto AddQuery::toString() -> std::string {
  return "QUERY = ADD " + this->targetTable + "\"";
}
