//
// Created by liu on 18-10-25.
//

#ifndef SRC_QUERY_DATA_UPDATEQUERY_H_
#define SRC_QUERY_DATA_UPDATEQUERY_H_

#include <string>

#include "../../db/Table.h"
#include "../Query.h"
#include "../QueryResult.h"

class UpdateQuery : public ComplexQuery {
  static constexpr const char *qname = "UPDATE";
  Table::ValueType
      fieldValue{};  // = (operands[0]=="KEY")? 0 :std::stoi(operands[1]);
  Table::FieldIndex fieldId{};
  Table::KeyType keyValue;

public:
  using ComplexQuery::ComplexQuery;

  auto execute() -> QueryResult::Ptr override;

  auto toString() -> std::string override;

  [[nodiscard]] auto type() const noexcept -> QueryType override {
    return QueryType::Update;
  }
};

#endif  // SRC_QUERY_DATA_UPDATEQUERY_H_
