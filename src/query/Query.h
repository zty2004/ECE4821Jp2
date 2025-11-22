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
  Load,        // WRITE
  Dump,        // READ
  Drop,        // WRITE
  Truncate,    // WRITE
  CopyTable,   // WRITE
  List,        // READ
  Quit,        // NULL
  PrintTable,  // READ
  Insert,      // WRITE
  Update,      // WRITE
  Select,      // READ
  Delete,      // WRITE
  Duplicate,   // WRITE
  Count,       // READ
  Sum,         // READ
  Min,         // READ
  Max,         // READ
  Add,         // WRITE
  Sub,         // WRITE
  Swap,        // WRITE
  Nop,         // NULL
};

struct QueryCondition {
  std::string field;
  size_t fieldId;
  std::string op;
  std::function<bool(const Table::ValueType &, const Table::ValueType &)> comp;
  std::string value;
  Table::ValueType valueParsed;
};

class Query {
protected:
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)
  std::string targetTable;
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)
  int id = -1;

public:
  Query() = default;

  explicit Query(std::string targetTable)
      : targetTable(std::move(targetTable)) {}

  using Ptr = std::unique_ptr<Query>;

  // Polymorphic base class - delete copy, default move
  Query(const Query &) = delete;
  auto operator=(const Query &) -> Query & = delete;
  Query(Query &&) = default;
  auto operator=(Query &&) -> Query & = default;

  virtual auto execute() -> QueryResult::Ptr = 0;

  virtual auto toString() -> std::string = 0;

  // cppcheck-suppress unusedFunction
  [[nodiscard]] virtual auto type() const noexcept -> QueryType {
    return QueryType::Nop;
  }

  // cppcheck-suppress unusedFunction
  [[nodiscard]] auto table() const -> const std::string & {
    return targetTable;
  }

  // Virtual accessor for an associated file path (LOAD/DUMP)
  // cppcheck-suppress unusedFunction
  [[nodiscard]] virtual auto filePath() const -> const std::string & {
    static const std::string kEmptyFilePath;
    return kEmptyFilePath;
  }

  // Virtual accessor for new table name (COPYTABLE)
  // cppcheck-suppress unusedFunction
  [[nodiscard]] virtual auto newTable() const -> const std::string & {
    static const std::string kEmptyNewTable;
    return kEmptyNewTable;
  }

  virtual ~Query() = default;
};

class NopQuery : public Query {
public:
  auto execute() -> QueryResult::Ptr override {
    return std::make_unique<NullQueryResult>();
  }

  [[nodiscard]] auto type() const noexcept -> QueryType override {
    return QueryType::Nop;
  }

  // cppcheck-suppress unusedFunction
  [[maybe_unused]] auto toString() -> std::string override {
    return "QUERY = NOOP";
  }
};

class ComplexQuery : public Query {
protected:
  /** The field names in the first () */
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)
  std::vector<std::string> operands;
  /** The function used in where clause */
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)
  std::vector<QueryCondition> condition;

public:
  using Ptr = std::unique_ptr<ComplexQuery>;

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

  ComplexQuery(std::string targetTable, std::vector<std::string> operands,
               std::vector<QueryCondition> condition)
      : Query(std::move(targetTable)), operands(std::move(operands)),
        condition(std::move(condition)) {}

  /** Get operands in the query */
  // cppcheck-suppress unusedFunction
  [[maybe_unused]] [[nodiscard]] auto
  getOperands() -> const std::vector<std::string> & {
    return operands;
  }

  /** Get condition in the query, seems no use now */
  // cppcheck-suppress unusedFunction
  [[maybe_unused]] auto getCondition() -> const std::vector<QueryCondition> & {
    return condition;
  }
};

#endif  // SRC_QUERY_QUERY_H_
