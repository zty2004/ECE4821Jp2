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
  size_t fieldId;
  std::string op;
  std::function<bool(const Table::ValueType &, const Table::ValueType &)> comp;
  std::string value;
  Table::ValueType valueParsed;
};

class Query {
protected:
  std::string targetTable;
  int id = -1;

public:
  Query() = default;

  explicit Query(std::string targetTable)
      : targetTable(std::move(targetTable)) {}

  typedef std::unique_ptr<Query> Ptr;

  virtual QueryResult::Ptr execute() = 0;

  virtual std::string toString() = 0;

  [[nodiscard]] virtual auto type() const noexcept -> QueryType {
    return QueryType::Nop;
  }

  [[nodiscard]] auto table() const -> const std::string & {
    return targetTable;
  }

  // Virtual accessor for an associated file path (LOAD/DUMP)
  [[nodiscard]] virtual auto filePath() const -> const std::string & {
    static const std::string kEmptyFilePath;
    return kEmptyFilePath;
  }

  virtual ~Query() = default;
};

class NopQuery : public Query {
public:
  QueryResult::Ptr execute() override {
    return std::make_unique<NullQueryResult>();
  }

  [[nodiscard]] auto type() const noexcept -> QueryType override {
    return QueryType::Nop;
  }

  // cppcheck-suppress unusedFunction
  [[maybe_unused]] std::string toString() override { return "QUERY = NOOP"; }
};

class ComplexQuery : public Query {
protected:
  /** The field names in the first () */
  std::vector<std::string> operands;
  /** The function used in where clause */
  std::vector<QueryCondition> condition;

public:
  typedef std::unique_ptr<ComplexQuery> Ptr;

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
  std::pair<std::string, bool> initCondition(const Table &table);

  /**
   * skip the evaluation of KEY
   * (which should be done after initConditionFast is called)
   * @param conditions
   * @param object
   * @return
   */
  bool evalCondition(const Table::Object &object);
  bool evalCondition(const Table::ConstObject &object);

  /**
   * This function seems have small effect and causes somme bugs
   * so it is not used actually
   * @param table
   * @param function
   * @return
   */
  [[maybe_unused]] bool testKeyCondition(
      const Table &table,
      const std::function<void(bool, Table::ConstObject::Ptr &&)> &function);

  ComplexQuery(std::string targetTable, std::vector<std::string> operands,
               std::vector<QueryCondition> condition)
      : Query(std::move(targetTable)), operands(std::move(operands)),
        condition(std::move(condition)) {}

  /** Get operands in the query */
  // cppcheck-suppress unusedFunction
  [[maybe_unused]] const std::vector<std::string> &getOperands() const {
    return operands;
  }

  /** Get condition in the query, seems no use now */
  // cppcheck-suppress unusedFunction
  [[maybe_unused]] const std::vector<QueryCondition> &getCondition() {
    return condition;
  }
};

#endif // SRC_QUERY_QUERY_H_
