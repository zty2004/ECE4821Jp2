//
// Created by liu on 18-10-25.
//

#ifndef SRC_QUERY_DATA_UPDATEQUERY_H_
#define SRC_QUERY_DATA_UPDATEQUERY_H_

#include "../Query.h"

class UpdateQuery : public ComplexQuery {
  static constexpr const char *qname = "UPDATE";
  Table::ValueType
      fieldValue{}; // = (operands[0]=="KEY")? 0 :std::stoi(operands[1]);
  Table::FieldIndex fieldId{};
  Table::KeyType keyValue{};

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // SRC_QUERY_DATA_UPDATEQUERY_H_
