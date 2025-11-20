//
// Created by liu on 18-10-25.
//

#include "Query.h"

#include <cassert>
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include "../db/Table.h"
#include "../utils/formatter.h"
#include "../utils/uexception.h"

auto ComplexQuery::initCondition(const Table &table)
    -> std::pair<std::string, bool> {
  constexpr int base_ten = 10;
  const std::unordered_map<std::string, int> opmap{
      {">", '>'}, {"<", '<'}, {"=", '='}, {">=", 'g'}, {"<=", 'l'},
  };
  std::pair<std::string, bool> result = {"", true};
  for (auto &cond : condition) {
    if (cond.field == "KEY") {
      if (cond.op != "=") {
        throw IllFormedQueryCondition("Can only compare equivalence on KEY");
      }
      if (result.first.empty()) {
        result.first = cond.value;
      } else if (result.first != cond.value) {
        result.second = false;
        return result;
      }
    } else {
      cond.fieldId = table.getFieldIndex(cond.field);
      cond.valueParsed = static_cast<Table::ValueType>(
          std::strtol(cond.value.c_str(), nullptr, base_ten));
      int oper = 0;
      try {
        oper = opmap.at(cond.op);
      } catch (const std::out_of_range &) {
        throw IllFormedQueryCondition(
            R"("?" is not a valid condition operator.)"_f % cond.op);
      }
      switch (oper) {
      case '>':
        cond.comp = std::greater<>();
        break;
      case '<':
        cond.comp = std::less<>();
        break;
      case '=':
        cond.comp = std::equal_to<>();
        break;
      case 'g':
        cond.comp = std::greater_equal<>();
        break;
      case 'l':
        cond.comp = std::less_equal<>();
        break;
      default:
        assert(0);  // should never be here
      }
    }
  }
  return result;
}

auto ComplexQuery::evalCondition(const Table::Object &object) -> bool {
  bool ret = true;
  for (const auto &cond : condition) {
    if (cond.field != "KEY") {
      ret = ret && cond.comp(object[cond.fieldId], cond.valueParsed);
    } else {
      ret = ret && (object.key() == cond.value);
    }
  }
  return ret;
}

auto ComplexQuery::evalCondition(const Table::ConstObject &object) -> bool {
  bool ret = true;
  for (const auto &cond : condition) {
    if (cond.field != "KEY") {
      ret = ret && cond.comp(object[cond.fieldId], cond.valueParsed);
    } else {
      ret = ret && (object.key() == cond.value);
    }
  }
  return ret;
}

auto ComplexQuery::testKeyCondition(
    const Table &table,
    const std::function<void(bool, Table::ConstObject::Ptr &&)> &function)
    -> bool {
  auto condResult = initCondition(table);
  if (!condResult.second) {
    function(false, nullptr);
    return true;
  }
  if (!condResult.first.empty()) {
    auto object = table[condResult.first];
    if (object != nullptr && evalCondition(*object)) {
      function(true, std::move(object));
    } else {
      function(false, nullptr);
    }
    return true;
  }
  return false;
}
