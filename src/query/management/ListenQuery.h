//
// ListenQuery
//

#ifndef SRC_QUERY_MANAGEMENT_LISTENQUERY_H_
#define SRC_QUERY_MANAGEMENT_LISTENQUERY_H_

#include <string>
#include <utility>

#include "../Query.h"

class ListenQuery : public Query {
  static constexpr const char *qname = "LISTEN";
  std::string fileName;

public:
  explicit ListenQuery(std::string fileName) : fileName(std::move(fileName)) {}

  auto execute() -> QueryResult::Ptr override;

  auto toString() -> std::string override;

  [[nodiscard]] auto type() const noexcept -> QueryType override {
    return QueryType::Listen;
  }

  [[nodiscard]] auto getFileName() const -> const std::string & {
    return fileName;
  }
};

#endif  // SRC_QUERY_MANAGEMENT_LISTENQUERY_H_
