//
// Created by liu on 18-10-23.
//

#include "Table.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

constexpr const Table::ValueType Table::ValueTypeMax;
constexpr const Table::ValueType Table::ValueTypeMin;

Table::FieldIndex
Table::getFieldIndex(const Table::FieldNameType &field) const {
  try {
    return this->fieldMap.at(field);
  } catch (const std::out_of_range &) {
    throw TableFieldNotFound(R"(Field name "?" doesn't exists.)"_f % (field));
  }
}

void Table::insertByIndex(const KeyType &key, std::vector<ValueType> &&data) {
  if (this->keyMap.find(key) != this->keyMap.end()) {
    std::string const err = "In Table \"" + this->tableName + "\" : Key \"" +
                            key + "\" already exists!";
    throw ConflictingKey(err);
  }
  this->keyMap.emplace(key, this->data.size());
  this->data.emplace_back(key, data);
}

bool Table::deleteByIndex(const KeyType &key) {
  auto it = keyMap.find(key);
  if (it == keyMap.end())
    return false;
  SizeType const del_ind = it->second;
  SizeType const last_ind = this->data.size() - 1;
  if (del_ind != last_ind) {
    std::swap(this->data[del_ind], this->data[last_ind]);

    // update the keyMap for the swapped element
    this->keyMap[this->data[del_ind].key] = del_ind;
  }
  this->data.pop_back();
  this->keyMap.erase(it);
  return true;
}

Table::Object::Ptr Table::operator[](const Table::KeyType &key) {
  auto it = keyMap.find(key);
  if (it == keyMap.end()) {
    // not found
    return nullptr;
  } else {
    return createProxy(
        data.begin() +
            static_cast<std::vector<Table::Datum>::difference_type>(it->second),
        this);
  }
}

Table::ConstObject::Ptr Table::operator[](const Table::KeyType &key) const {
  auto it = keyMap.find(key);
  if (it == keyMap.end()) {
    // not found
    return nullptr;
  } else {
    return createProxy(
        data.cbegin() +
            static_cast<std::vector<Table::Datum>::difference_type>(it->second),
        this);
  }
}

std::ostream &operator<<(std::ostream &os, const Table &table) {
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
