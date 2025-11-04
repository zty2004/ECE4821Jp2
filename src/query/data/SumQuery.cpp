#include "SumQuery.h"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../utils/formatter.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"

auto SumQuery::execute() -> QueryResult::Ptr {
  Database &database = Database::getInstance();
  try {
    auto &table = database[this->targetTable];

    // check whether operands are provided
    if (this->operands.empty()) {
      return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                              "No operands (? operands)."_f %
                                                  operands.size());
    }

    // KEY field cannot be summed over
    if (std::ranges::any_of(
            this->operands,
            [](const std::string &fname) -> bool { return fname == "KEY"; })) {
      return std::make_unique<ErrorMsgResult>(
          qname, this->targetTable, "The KEY field cannot be summed over.");
    }

    // get field indices for all operands (ranges view + copy)
    fieldId.clear();
    fieldId.resize(this->operands.size());
    auto ids_view =
        this->operands |
        std::ranges::views::transform(
            [&table](const std::string &fname) -> Table::FieldIndex {
              return table.getFieldIndex(fname);
            });
    std::ranges::copy(ids_view, fieldId.begin());

    // if no record is affected, then the sum is set to zero
    // use int64_t to avoid overflow during accumulation
    std::vector<int64_t> sums(fieldId.size(), 0);

    auto result = initCondition(table);
    if (result.second) {
      // iterate through all datum and sum the one satisfying condition
      for (auto &&obj : table) {
        if (this->evalCondition(obj)) {
          // pairwise transform: sums[i] = sums[i] + obj[fieldId[i]]
          std::ranges::transform(
              fieldId, sums, sums.begin(),
              [&obj](Table::FieldIndex fid, int64_t acc) -> int64_t {
                return acc + static_cast<int64_t>(obj[fid]);
              });
        }
      }
    }
    // Convert back to int for the result
    std::vector<int> result_sums(sums.size());
    std::ranges::transform(sums, result_sums.begin(), [](int64_t val) -> int {
      return static_cast<int>(val);
    });
    return std::make_unique<SuccessMsgResult>(result_sums);
  } catch (const TableNameNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const TableFieldNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const IllFormedQueryCondition &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const std::invalid_argument &e) {
    // Cannot convert operand to string
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            "Unknown error '?'"_f % e.what());
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            "Unknown error '?'"_f % e.what());
  }
}

auto SumQuery::toString() -> std::string {
  return "QUERY = SUM FROM " + this->targetTable + "\"";
}
