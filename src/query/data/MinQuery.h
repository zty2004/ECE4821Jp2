#ifndef PROJECT_MINQUERY_H
#define PROJECT_MINQUERY_H

#include <string>
#include <vector>

#include "../Query.h"
#include "../QueryResult.h"

class MinQuery : public ComplexQuery {
  static constexpr const char *qname = "MIN";

  std::vector<Table::FieldIndex> fieldId;

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // PROJECT_MINQUERY_H
