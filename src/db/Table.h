//
// Created by liu on 18-10-23.
//

#ifndef SRC_DB_TABLE_H_
#define SRC_DB_TABLE_H_

#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../utils/formatter.h"
#include "../utils/uexception.h"

#define DBTABLE_ACCESS_WITH_NAME_EXCEPTION(field)                              \
  do {                                                                         \
    try {                                                                      \
      auto &index = table->fieldMap.at(field);                                 \
      return it->datum.at(index);                                              \
    } catch (const std::out_of_range &) {                                      \
      throw TableFieldNotFound(R"(Field name "?" doesn't exists.)"_f %         \
                               (field));                                       \
    }                                                                          \
  } while (0)

#define DBTABLE_ACCESS_WITH_INDEX_EXCEPTION(index)                             \
  do {                                                                         \
    try {                                                                      \
      return it->datum.at(index);                                              \
    } catch (const std::out_of_range &) {                                      \
      throw TableFieldNotFound(R"(Field index ? out of range.)"_f % (index));  \
    }                                                                          \
  } while (0)

class Table {
public:
  using KeyType = std::string;
  using FieldNameType = std::string;
  using FieldIndex = size_t;
  using ValueType = int;
  static constexpr const ValueType ValueTypeMax =
      std::numeric_limits<ValueType>::max();
  static constexpr const ValueType ValueTypeMin =
      std::numeric_limits<ValueType>::min();
  using SizeType = size_t;

private:
  /** A row in the table */
  struct Datum {
  private:
    /** Unique key of this datum */
    KeyType key;
    /** The values in the order of fields */
    std::vector<ValueType> datum;

  public:
    friend class Table;
    template <class Iterator, class VType> friend class ObjectImpl;
    friend auto operator<<(std::ostream &os,
                           const Table &table) -> std::ostream &;

    Datum() = default;

    Datum(const Datum &) = default;
    auto operator=(const Datum &) -> Datum & = default;
    Datum(Datum &&) noexcept = default;
    auto operator=(Datum &&) noexcept -> Datum & = default;

    ~Datum() = default;

    explicit Datum(const SizeType &size) : datum(size, ValueType()) {}

    template <class ValueTypeContainer>
    explicit Datum(KeyType key, const ValueTypeContainer &datum)
        : key(std::move(key)), datum(datum) {}

    template <class K>
    explicit Datum(K &&key, std::vector<ValueType> &&datum)
        : key(std::forward<K>(key)), datum(std::move(datum)) {}
  };

  using DataIterator = std::vector<Datum>::iterator;
  using ConstDataIterator = std::vector<Datum>::const_iterator;

  /** The fields, ordered as defined in fieldMap */
  std::vector<FieldNameType> fields;
  /** Map field name into index */
  std::unordered_map<FieldNameType, FieldIndex> fieldMap;

  /** The rows are saved in a vector, which is unsorted */
  std::vector<Datum> data;
  /** Used to keep the keys unique and provide O(1) access with key */
  std::unordered_map<KeyType, SizeType> keyMap;

  /** The name of table */
  std::string tableName;

public:
  using Ptr = std::unique_ptr<Table>;

  /**
   * A proxy class that provides abstraction on internal Implementation.
   * Allows independent variation on the representation for a table object
   *
   * @tparam Iterator
   * @tparam VType
   */
  template <class Iterator, class VType> class ObjectImpl {
    friend class Table;

    /** Not const because key can be updated */
    Iterator it;
    Table *table;

  public:
    using Ptr = std::unique_ptr<ObjectImpl>;

    ObjectImpl(Iterator datumIt, const Table *table_ptr)
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        : it(datumIt), table(const_cast<Table *>(table_ptr)) {}

    ObjectImpl(const ObjectImpl &other) = default;

    ObjectImpl(ObjectImpl &&other) noexcept = default;

    auto operator=(const ObjectImpl &) -> ObjectImpl & = default;

    auto operator=(ObjectImpl &&) noexcept -> ObjectImpl & = default;

    ~ObjectImpl() = default;

    [[nodiscard]] auto key() const -> const KeyType & { return it->key; }

    void setKey(KeyType key) {
      auto keyMapIt = table->keyMap.find(it->key);
      auto dataIt = std::move(keyMapIt->second);
      table->keyMap.erase(keyMapIt);
      table->keyMap.emplace(key, std::move(dataIt));
      it->key = std::move(key);
    }

    /**
     * Accessing by index should be, at least as fast as accessing by field
     * name. Clients should prefer accessing by index if the same field is
     * accessed frequently (the implement is improved so that index is actually
     * faster now)
     */
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while)
    auto operator[](const FieldNameType &field) const -> VType & {
      DBTABLE_ACCESS_WITH_NAME_EXCEPTION(field);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while)
    auto operator[](const FieldIndex &index) const -> VType & {
      DBTABLE_ACCESS_WITH_INDEX_EXCEPTION(index);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while)
    [[nodiscard]] auto get(const FieldNameType &field) const -> VType & {
      DBTABLE_ACCESS_WITH_NAME_EXCEPTION(field);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while)
    [[nodiscard]] auto get(const FieldIndex &index) const -> VType & {
      DBTABLE_ACCESS_WITH_INDEX_EXCEPTION(index);
    }
  };

  using Object = ObjectImpl<DataIterator, ValueType>;
  using ConstObject = ObjectImpl<ConstDataIterator, const ValueType>;

  /**
   * A proxy class that provides iteration on the table
   * @tparam ObjType
   * @tparam DatumIterator
   */
  template <typename ObjType, typename DatumIterator> class IteratorImpl {
    friend class Table;

    DatumIterator it;
    const Table *table = nullptr;

  public:
    using difference_type = std::ptrdiff_t;
    using value_type = ObjType;
    using pointer = typename ObjType::Ptr;
    using reference = ObjType;
    using iterator_category = std::random_access_iterator_tag;
    using iterator_concept = std::random_access_iterator_tag;
    // See https://stackoverflow.com/questions/37031805/

    IteratorImpl(DatumIterator datumIt, const Table *table_ptr)
        : it(datumIt), table(table_ptr) {}

    IteratorImpl() = default;

    IteratorImpl(const IteratorImpl &) = default;

    IteratorImpl(IteratorImpl &&) noexcept = default;

    auto operator=(const IteratorImpl &) -> IteratorImpl & = default;

    auto operator=(IteratorImpl &&) noexcept -> IteratorImpl & = default;

    ~IteratorImpl() = default;

    auto operator->() -> pointer { return createProxy(it, table); }

    // *Note: Directly build an Obj instead of Proxy, need further test
    auto operator*() -> reference { return ObjType(it, table); }

    auto operator+(difference_type n) const -> IteratorImpl {
      return IteratorImpl(it + n, table);
    }

    auto operator-(difference_type n) const -> IteratorImpl {
      return IteratorImpl(it - n, table);
    }

    auto operator-(const IteratorImpl &other) const -> difference_type {
      return this->it - other.it;
    }

    auto operator[](difference_type n) const -> reference {
      return *(*this + n);
    }

    auto operator+=(difference_type n) -> IteratorImpl & {
      return it += n, *this;
    }

    auto operator-=(difference_type n) -> IteratorImpl & {
      return it -= n, *this;
    }

    auto operator++() -> IteratorImpl & { return ++it, *this; }

    auto operator--() -> IteratorImpl & { return --it, *this; }

    auto operator++(int) -> IteratorImpl {
      auto retVal = IteratorImpl(*this);
      ++it;
      return retVal;
    }

    auto operator--(int) -> IteratorImpl {
      auto retVal = IteratorImpl(*this);
      --it;
      return retVal;
    }

    auto operator==(const IteratorImpl &other) const -> bool {
      return this->it == other.it;
    }

    auto operator!=(const IteratorImpl &other) const -> bool {
      return this->it != other.it;
    }

    auto operator<=(const IteratorImpl &other) const -> bool {
      return this->it <= other.it;
    }

    auto operator>=(const IteratorImpl &other) const -> bool {
      return this->it >= other.it;
    }

    auto operator<(const IteratorImpl &other) const -> bool {
      return this->it < other.it;
    }

    auto operator>(const IteratorImpl &other) const -> bool {
      return this->it > other.it;
    }
  };

  using Iterator = IteratorImpl<Object, decltype(data.begin())>;
  using ConstIterator = IteratorImpl<ConstObject, decltype(data.cbegin())>;

private:
  static auto createProxy(ConstDataIterator it,
                          const Table *table) -> ConstObject::Ptr {
    return std::make_unique<ConstObject>(it, table);
  }

  static auto createProxy(DataIterator it, const Table *table) -> Object::Ptr {
    return std::make_unique<Object>(it, table);
  }

public:
  Table() = delete;

  explicit Table(std::string name) : tableName(std::move(name)) {}

  /**
   * Accept any container that contains string.
   * @tparam FieldIDContainer
   * @param name: the table name (must be unique in the database)
   * @param fields: an iterable container with fields
   */
  template <class FieldIDContainer>
  Table(const std::string &name, const FieldIDContainer &fields);

  /**
   * Copy constructor from another table
   * @param name: the table name (must be unique in the database)
   * @param origin: the original table copied from
   */
  Table(std::string name, const Table &origin)
      : fields(origin.fields), fieldMap(origin.fieldMap), data(origin.data),
        keyMap(origin.keyMap), tableName(std::move(name)) {}

  /**
   * Find the index of a field in the fieldMap
   * @param field
   * @return fieldIndex
   */
  [[nodiscard]] auto
  getFieldIndex(const FieldNameType &field) const -> FieldIndex;

  /**
   * Insert a row of data by its key
   * @tparam ValueTypeContainer
   * @param key
   * @param data
   */
  void insertByIndex(const KeyType &key, std::vector<ValueType> &&data);

  /**
   * Delete a row by its key
   * @param key
   * @return true if the row was deleted, false if key doesn't exist
   */
  auto deleteByIndex(const KeyType &key) -> bool;

  /**
   * Duplicate a row by source key to a destination key.
   * @param src Source key to copy from (must exist)
   * @param dst Destination key to create (must not exist)
   * @return true if duplicated successfully, false if src not found or dst
   * already exists
   */
  auto duplicateByKey(const KeyType &src, const KeyType &dst) -> bool;

  /**
   * Access the value according to the key
   * @param key
   * @return the Object that KEY = key, or nullptr if key doesn't exist
   */
  auto operator[](const KeyType &key) -> Object::Ptr;

  /**
   * Const access the value according to the key
   * @param key
   * @return the ConstObject that KEY = key, or nullptr if key doesn't exist
   */
  auto operator[](const KeyType &key) const -> ConstObject::Ptr;

  /**
   * Set the name of the table
   * @param name
   */
  [[maybe_unused]] void setName(std::string name) {
    this->tableName = std::move(name);
  }

  /**
   * Get the name of the table
   * @return
   */
  [[nodiscard]] auto name() const -> const std::string & {
    return this->tableName;
  }

  /**
   * Return whether the table is empty
   * @return
   */
  [[nodiscard]] auto empty() const -> bool { return this->data.empty(); }

  /**
   * Return the num of data stored in the table
   * @return
   */
  [[nodiscard]] auto size() const -> size_t { return this->data.size(); }

  /**
   * Return the fields in the table
   * @return
   */
  [[nodiscard]] auto field() const -> const std::vector<FieldNameType> & {
    return this->fields;
  }

  /**
   * Clear all content in the table
   * @return rows affected
   */
  auto clear() -> size_t {
    auto result = keyMap.size();
    data.clear();
    keyMap.clear();
    return result;
  }

  /**
   * Get a begin iterator similar to the standard iterator
   * @return begin iterator
   */
  auto begin() noexcept -> Iterator { return {data.begin(), this}; }

  /**
   * Get a end iterator similar to the standard iterator
   * @return end iterator
   */
  auto end() noexcept -> Iterator { return {data.end(), this}; }

  /**
   * Get a const begin iterator similar to the standard iterator
   * @return const begin iterator
   */
  [[nodiscard]] auto begin() const noexcept -> ConstIterator {
    return {data.cbegin(), this};
  }

  /**
   * Get a const end iterator similar to the standard iterator
   * @return const end iterator
   */
  [[nodiscard]] auto end() const noexcept -> ConstIterator {
    return {data.cend(), this};
  }

  /**
   * Overload the << operator for complete print of the table
   * @param os
   * @param table
   * @return the origin ostream
   */
  friend auto operator<<(std::ostream &os,
                         const Table &table) -> std::ostream &;
};

auto operator<<(std::ostream &os, const Table &table) -> std::ostream &;

template <class FieldIDContainer>
Table::Table(const std::string &name, const FieldIDContainer &fields)
    : fields(fields.cbegin(), fields.cend()), tableName(name) {
  SizeType fieldIndex = 0;
  for (const auto &fieldName : fields) {
    if (fieldName == "KEY") {
      throw MultipleKey("Error creating table \"" + name +
                        "\": Multiple KEY field.");
    }
    fieldMap.emplace(fieldName, fieldIndex++);
  }
}

#endif  // SRC_DB_TABLE_H_
