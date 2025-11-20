//
// Created by liu on 18-10-25.
//

#include "QueryBuilders.h"

#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../db/Database.h"
#include "../utils/formatter.h"
#include "../utils/uexception.h"
#include "Query.h"
#include "QueryParser.h"
#include "data/AddQuery.h"
#include "data/CountQuery.h"
#include "data/DeleteQuery.h"
#include "data/DuplicateQuery.h"
#include "data/InsertQuery.h"
#include "data/MaxQuery.h"
#include "data/MinQuery.h"
#include "data/SelectQuery.h"
#include "data/SubQuery.h"
#include "data/SumQuery.h"
#include "data/SwapQuery.h"
#include "data/UpdateQuery.h"
#include "management/CopyTableQuery.h"
#include "management/DropTableQuery.h"
#include "management/DumpTableQuery.h"
#include "management/ListTableQuery.h"
#include "management/ListenQuery.h"
#include "management/LoadTableQuery.h"
#include "management/PrintTableQuery.h"
#include "management/QuitQuery.h"
#include "management/TruncateTableQuery.h"

// Prints out debugging information.
// Does no real work
auto FakeQueryBuilder::tryExtractQuery(const TokenizedQueryString &query)
    -> Query::Ptr {
  constexpr int column_width = 10;
  constexpr int columns_per_row = 5;
  constexpr int wrap_column = 4;
  std::cerr << "Query string: \n" << query.rawQeuryString << "\n";
  std::cerr << "Tokens:\n";
  int count = 0;
  for (const auto &tok : query.token) {
    std::cerr << std::setw(column_width) << "\"" << tok << "\"";
    count = (count + 1) % columns_per_row;
    if (count == wrap_column) {
      std::cerr << '\n';
    }
  }
  if (count != wrap_column) {
    std::cerr << '\n';
  }
  return this->nextBuilder->tryExtractQuery(query);
}

auto ManageTableQueryBuilder::tryExtractQuery(const TokenizedQueryString &query)
    -> Query::Ptr {
  if (query.token.size() == 2) {
    if (query.token.front() == "LOAD") {
      auto &database = Database::getInstance();
      auto tableName = database.getFileTableName(query.token[1]);
      return std::make_unique<LoadTableQuery>(tableName, query.token[1]);
    }
    if (query.token.front() == "DROP") {
      return std::make_unique<DropTableQuery>(query.token[1]);
    }
    if (query.token.front() == "TRUNCATE") {
      return std::make_unique<TruncateTableQuery>(query.token[1]);
    }
    if (query.token.front() == "LISTEN") {
      return std::make_unique<ListenQuery>(query.token[1]);
    }
  }
  if (query.token.size() == 3) {
    if (query.token.front() == "DUMP") {
      auto &database = Database::getInstance();
      database.updateFileTableName(query.token[2], query.token[1]);
      return std::make_unique<DumpTableQuery>(query.token[1], query.token[2]);
    }
    if (query.token.front() == "COPYTABLE") {
      return std::make_unique<CopyTableQuery>(query.token[1], query.token[2]);
    }
  }
  return this->nextBuilder->tryExtractQuery(query);
}

auto DebugQueryBuilder::tryExtractQuery(const TokenizedQueryString &query)
    -> Query::Ptr {
  if (query.token.size() == 1) {
    if (query.token.front() == "LIST") {
      return std::make_unique<ListTableQuery>();
    }
    if (query.token.front() == "QUIT") {
      return std::make_unique<QuitQuery>();
    }
  }
  if (query.token.size() == 2) {
    if (query.token.front() == "SHOWTABLE") {
      return std::make_unique<PrintTableQuery>(query.token[1]);
    }
  }
  return BasicQueryBuilder::tryExtractQuery(query);
}

void ComplexQueryBuilder::parseOperands(
    std::vector<std::string>::const_iterator *iter,
    const std::vector<std::string>::const_iterator &end) {
  if (**iter != "(") {
    throw IllFormedQuery("Ill-formed operand.");
  }
  ++(*iter);
  while (**iter != ")") {
    this->operandToken.push_back(**iter);
    ++(*iter);
    if (*iter == end) {
      throw IllFormedQuery("Ill-formed operand");
    }
  }
  ++(*iter);
}

void ComplexQueryBuilder::parseWhereConditions(
    std::vector<std::string>::const_iterator *iter,
    const std::vector<std::string>::const_iterator &end) {
  while (*iter != end) {
    if (**iter != "(") {
      throw IllFormedQuery("Ill-formed query condition");
    }
    QueryCondition cond;
    cond.fieldId = 0;
    cond.valueParsed = 0;
    // cppcheck-suppress knownConditionTrueFalse
    if (++(*iter) == end) {
      throw IllFormedQuery("Missing field in condition");
    }
    cond.field = **iter;
    if (++(*iter) == end) {
      throw IllFormedQuery("Missing operator in condition");
    }
    cond.op = **iter;
    if (++(*iter) == end) {
      throw IllFormedQuery("Missing  in condition");
    }
    cond.value = **iter;
    if (++(*iter) == end || **iter != ")") {
      throw IllFormedQuery("Ill-formed query condition");
    }
    this->conditionToken.push_back(cond);
    ++(*iter);
  }
}

void ComplexQueryBuilder::parseToken(const TokenizedQueryString &query) {
  // Treats forms like:
  //
  // $OPER$ ( arg1 arg2 ... )
  // FROM table
  // WHERE ( KEY = $STR$ ) ( $field$ $OP$ $int$ ) ...
  //
  // The "WHERE" clause can be ommitted
  // The args of OPER clause can be ommitted

  auto iter = query.token.cbegin();
  auto end = query.token.cend();
  iter += 1;  // Take to args;
  if (iter == query.token.end()) {
    throw IllFormedQuery("Missing FROM clause");
  }
  if (*iter != "FROM") {
    parseOperands(&iter, end);
    if (iter == end || *iter != "FROM") {
      throw IllFormedQuery("Missing FROM clause");
    }
  }
  if (++iter == end) {
    throw IllFormedQuery("Missing targed table");
  }
  this->targetTable = *iter;
  if (++iter == end) {  // the "WHERE" clause is ommitted
    return;
  }
  if (*iter != "WHERE") {
    // Hmmm, C++11 style Raw-string literal
    // Reference:
    // http://en.cppreference.com/w/cpp/language/string_literal
    throw IllFormedQuery(R"(Expecting "WHERE", found "?".)"_f % *iter);
  }
  ++iter;
  parseWhereConditions(&iter, end);
}

auto ComplexQueryBuilder::tryExtractQuery(const TokenizedQueryString &query)
    -> Query::Ptr {
  try {
    this->parseToken(query);
  } catch (const IllFormedQuery &exception) {
    std::cerr << exception.what() << '\n';
    return this->nextBuilder->tryExtractQuery(query);
  }
  std::string const operation = query.token.front();
  if (operation == "INSERT") {
    return std::make_unique<InsertQuery>(this->targetTable, this->operandToken,
                                         this->conditionToken);
  }
  if (operation == "UPDATE") {
    return std::make_unique<UpdateQuery>(this->targetTable, this->operandToken,
                                         this->conditionToken);
  }
  if (operation == "SELECT") {
    return std::make_unique<SelectQuery>(this->targetTable, this->operandToken,
                                         this->conditionToken);
  }
  if (operation == "DELETE") {
    return std::make_unique<DeleteQuery>(this->targetTable, this->operandToken,
                                         this->conditionToken);
  }
  if (operation == "DUPLICATE") {
    return std::make_unique<DuplicateQuery>(
        this->targetTable, this->operandToken, this->conditionToken);
  }
  if (operation == "COUNT") {
    return std::make_unique<CountQuery>(this->targetTable, this->operandToken,
                                        this->conditionToken);
  }
  if (operation == "SUM") {
    return std::make_unique<SumQuery>(this->targetTable, this->operandToken,
                                      this->conditionToken);
  }
  if (operation == "MIN") {
    return std::make_unique<MinQuery>(this->targetTable, this->operandToken,
                                      this->conditionToken);
  }
  if (operation == "MAX") {
    return std::make_unique<MaxQuery>(this->targetTable, this->operandToken,
                                      this->conditionToken);
  }
  if (operation == "ADD") {
    return std::make_unique<AddQuery>(this->targetTable, this->operandToken,
                                      this->conditionToken);
  }
  if (operation == "SUB") {
    return std::make_unique<SubQuery>(this->targetTable, this->operandToken,
                                      this->conditionToken);
  }
  if (operation == "SWAP") {
    return std::make_unique<SwapQuery>(this->targetTable, this->operandToken,
                                       this->conditionToken);
  }
  std::cerr << "Complicated query found!" << '\n';
  std::cerr << "Operation = " << query.token.front() << '\n';
  std::cerr << "    Operands : ";
  for (const auto &oprand : this->operandToken) {
    std::cerr << oprand << " ";
  }
  std::cerr << '\n';
  std::cerr << "Target Table = " << this->targetTable << '\n';
  if (this->conditionToken.empty()) {
    std::cerr << "No WHERE clause specified." << '\n';
  } else {
    std::cerr << "Conditions = ";
  }
  for (const auto &cond : this->conditionToken) {
    std::cerr << cond.field << cond.op << cond.value << " ";
  }
  std::cerr << '\n';

  return this->nextBuilder->tryExtractQuery(query);
}

void ComplexQueryBuilder::clear() {
  this->conditionToken.clear();
  this->targetTable = "";
  this->operandToken.clear();
  this->nextBuilder->clear();
}
