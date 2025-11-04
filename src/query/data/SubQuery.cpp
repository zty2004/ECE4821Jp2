#include "SubQuery.h"

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

auto SubQuery::execute() -> QueryResult::Ptr {
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

    // Iterate and update: dst = src[0] - sum(src[1..])
    std::size_t counter = 0;
    if (condInit.second) {
      for (auto &&obj : table) {
        if (evalCondition(obj)) {
          int64_t value = obj[srcId[0]];
          // subtract the sum of remaining sources using std::accumulate
          // use int64_t to avoid overflow during accumulation
          int64_t const sub_sum =
              std::accumulate(srcId.begin() + 1, srcId.end(), 0LL,
                              [&obj](int64_t acc, size_t idx) -> int64_t {
                                return acc + obj[idx];
                              });
          value -= sub_sum;
          obj[dstId] = static_cast<int>(value);
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

auto SubQuery::toString() -> std::string {
  return "QUERY = SUB " + this->targetTable + "\"";
}
