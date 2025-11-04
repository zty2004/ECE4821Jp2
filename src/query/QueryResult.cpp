//
// Created by liu on 18-10-26.
//

#include "QueryResult.h"

#include <ostream>

auto operator<<(std::ostream &os, const QueryResult &table) -> std::ostream & {
  return table.output(os);
}
