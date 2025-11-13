//
// Created by liu on 18-10-25.
//

#ifndef SRC_QUERY_QUERY_H_
#define SRC_QUERY_QUERY_H_

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../db/Table.h"
#include "QueryResult.h"

// Type of Queries
enum class QueryType : std::uint8_t {
  Load,
  Dump,
  Drop,
  Truncate,
  CopyTable,
  List,
  Quit,
  PrintTable,
  Insert,
  Update,
  Select,
  Delete,
  Duplicate,
  Count,
  Sum,
  Min,
  Max,
  Add,
  Sub,
  Swap,
  Nop,
};

struct QueryCondition {
  std::string field;
  size_t fieldId{};  // NOLINT(cppcoreguidelines-pro-type-member-init)
  std::string op;
  std::function<bool(const Table::ValueType &, const Table::ValueType &)> comp;
  std::string value;
  Table::ValueType
      valueParsed{};  // NOLINT(cppcoreguidelines-pro-type-member-init)
};

class Query {
public:
  Query() = default;

  explicit Query(std::string targetTable)
      : targetTable(std::move(targetTable)) {}

  // Rule of five
  Query(const Query &) = delete;
  auto operator=(const Query &) -> Query & = delete;
  Query(Query &&) = delete;
  auto operator=(Query &&) -> Query & = delete;
  virtual ~Query() = default;

  using Ptr = std::unique_ptr<Query>;

  virtual auto execute() -> QueryResult::Ptr = 0;

  virtual auto toString() -> std::string = 0;

  [[nodiscard]] auto getTargetTable() const noexcept -> const std::string & {
    return targetTable;
  }

protected:
  std::string targetTable;  // NOLINT
  int id = -1;              // NOLINT
};

class NopQuery : public Query {
public:
  auto execute() -> QueryResult::Ptr override {
    return std::make_unique<NullQueryResult>();
  }

  [[maybe_unused]] auto toString() -> std::string override {
    return "QUERY = NOOP";
  }
};

class ComplexQuery : public Query {
public:
  using Ptr = std::unique_ptr<ComplexQuery>;

  ComplexQuery(std::string targetTable, std::vector<std::string> operands,
               std::vector<QueryCondition> condition)
      : Query(std::move(targetTable)), operands(std::move(operands)),
        condition(std::move(condition)) {}

protected:
  /** The field names in the first () */
  std::vector<std::string> operands;  // NOLINT
  /** The function used in where clause */
  std::vector<QueryCondition> condition;  // NOLINT

  /**
   * init a fast condition according to the table
   * note that the condition is only effective if the table fields are not
   * changed
   * @param table
   * @param conditions
   * @return a pair of the key and a flag
   * if flag is false, the condition is always false
   * in this situation, the condition may not be fully initialized to save time
   */
  auto initCondition(const Table &table) -> std::pair<std::string, bool>;

  /**
   * skip the evaluation of KEY
   * (which should be done after initConditionFast is called)
   * @param conditions
   * @param object
   * @return
   */
  auto evalCondition(const Table::Object &object) -> bool;
  auto evalCondition(const Table::ConstObject &object) -> bool;

  /**
   * This function seems have small effect and causes somme bugs
   * so it is not used actually
   * @param table
   * @param function
   * @return
   */
  [[maybe_unused]] auto testKeyCondition(
      const Table &table,
      const std::function<void(bool, Table::ConstObject::Ptr &&)> &function)
      -> bool;

  /** Get operands in the query */
  // cppcheck-suppress unusedFunction
  [[nodiscard]] [[maybe_unused]] auto getOperands() const
      -> const std::vector<std::string> & {
    return operands;
  }

  /** Get condition in the query, seems no use now */
  // cppcheck-suppress unusedFunction
  [[maybe_unused]] auto getCondition() -> const std::vector<QueryCondition> & {
    return condition;
  }
};

#endif  // SRC_QUERY_QUERY_H_
