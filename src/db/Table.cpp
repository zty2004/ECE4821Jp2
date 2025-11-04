//
// Created by liu on 18-10-23.
//

#include "Table.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../utils/formatter.h"
#include "../utils/uexception.h"

auto Table::getFieldIndex(const Table::FieldNameType &field) const
    -> Table::FieldIndex {
  try {
    return this->fieldMap.at(field);
  } catch (const std::out_of_range &) {
    throw TableFieldNotFound(R"(Field name "?" doesn't exists.)"_f % (field));
  }
}

void Table::insertByIndex(const KeyType &key, std::vector<ValueType> &&data) {
  if (this->keyMap.contains(key)) {
    std::string const err = "In Table \"" + this->tableName + "\" : Key \"" +
                            key + "\" already exists!";
    throw ConflictingKey(err);
  }
  this->keyMap.emplace(key, this->data.size());
  this->data.emplace_back(key, std::move(data));
}

auto Table::deleteByIndex(const KeyType &key) -> bool {
  auto iter = keyMap.find(key);
  if (iter == keyMap.end()) {
    return false;
  }
  SizeType const del_ind = iter->second;
  SizeType const last_ind = this->data.size() - 1;
  if (del_ind != last_ind) {
    std::swap(this->data[del_ind], this->data[last_ind]);

    // update the keyMap for the swapped element
    this->keyMap[this->data[del_ind].key] = del_ind;
  }
  this->data.pop_back();
  this->keyMap.erase(iter);
  return true;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto Table::duplicateByKey(const Table::KeyType &src, const Table::KeyType &dst)
    -> bool {
  if (this->keyMap.contains(dst)) {
    return false;
  }
  auto srcIt = this->keyMap.find(src);
  if (srcIt == this->keyMap.end()) {
    return false;
  }
  // Snapshot source row values
  const auto &srcDatum = this->data[srcIt->second];
  std::vector<ValueType> copyValues = srcDatum.datum;
  // Insert new row with destination key
  this->insertByIndex(dst, std::move(copyValues));
  return true;
}

auto Table::operator[](const Table::KeyType &key) -> Table::Object::Ptr {
  auto iter = keyMap.find(key);
  if (iter == keyMap.end()) {
    // not found
    return nullptr;
  }
  return createProxy(
      data.begin() +
          static_cast<std::vector<Table::Datum>::difference_type>(iter->second),
      this);
}

auto Table::operator[](const Table::KeyType &key) const
    -> Table::ConstObject::Ptr {
  auto iter = keyMap.find(key);
  if (iter == keyMap.end()) {
    // not found
    return nullptr;
  }
  return createProxy(
      data.cbegin() +
          static_cast<std::vector<Table::Datum>::difference_type>(iter->second),
      this);
}

auto operator<<(std::ostream &os, const Table &table) -> std::ostream & {
  const int width = 10;
  std::stringstream buffer;
  buffer << table.tableName << "\t" << (table.fields.size() + 1) << "\n";
  buffer << std::setw(width) << "KEY";
  for (const auto &field : table.fields) {
    buffer << std::setw(width) << field;
  }
  buffer << "\n";
  auto numFields = table.fields.size();
  for (const auto &datum : table.data) {
    buffer << std::setw(width) << datum.key;
    for (decltype(numFields) i = 0; i < numFields; ++i) {
      buffer << std::setw(width) << datum.datum[i];
    }
    buffer << "\n";
  }
  return os << buffer.str();
}
