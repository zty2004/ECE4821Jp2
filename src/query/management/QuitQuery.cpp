//
// Created by liu on 18-10-25.
//

#include "QuitQuery.h"

#include <memory>
#include <string>

#include "../../db/Database.h"
#include "../../query/QueryResult.h"

auto QuitQuery::toString() -> std::string { return "QUERY = Quit"; }

auto QuitQuery::execute() -> QueryResult::Ptr {
  // Database::exit() will throw QuitException
  // This exception will propagate up to main() for clean shutdown
  Database::exit();
  // This line will never be reached due to exception
  return std::make_unique<SuccessMsgResult>(qname);
}
