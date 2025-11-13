//
// Created by liu on 18-10-23.
//

#ifndef SRC_DB_DATABASE_H_
#define SRC_DB_DATABASE_H_

#include <string>
#include <unordered_map>

#include "Table.h"

class Database {
private:
  // The singleton instance is provided via Meyers Singleton.
  // No mutable global pointer member is kept inside the class.

  /**
   * The map of tableName -> table unique ptr
   */
  std::unordered_map<std::string, Table::Ptr> tables;

  /**
   * The map of fileName -> tableName
   */
  std::unordered_map<std::string, std::string> fileTableNameMap;

  /**
   * The default constructor is made private for singleton instance
   */
  Database() = default;

public:
  void testDuplicate(const std::string &tableName);

  auto registerTable(Table::Ptr &&table) -> Table &;

  void dropTable(const std::string &tableName);

  void printAllTable();

  auto operator[](const std::string &tableName) -> Table &;

  auto operator[](const std::string &tableName) const -> const Table &;

  auto operator=(const Database &) -> Database & = delete;

  auto operator=(Database &&) -> Database & = delete;

  Database(const Database &) = delete;

  Database(Database &&) = delete;

  ~Database() = default;

  static auto getInstance() -> Database &;

  void updateFileTableName(const std::string &fileName,
                           const std::string &tableName);

  auto getFileTableName(const std::string &fileName) -> std::string;

  /**
   * Load a table from an input stream (i.e., a file)
   * @param input_stream
   * @param source
   * @return reference of loaded table
   */
  static auto loadTableFromStream(std::istream &input_stream,
                                  const std::string &source = "") -> Table &;

  static void exit();
};

#endif  // SRC_DB_DATABASE_H_
