#ifndef NET_PROTO2_UTIL_CONVERTER_INTERNAL_DEFAULT_VALUE_OBJECTWRITER_H_
#define NET_PROTO2_UTIL_CONVERTER_INTERNAL_DEFAULT_VALUE_OBJECTWRITER_H_

#include <memory>
#include <stack>
#include <vector>

#include "base/macros.h"
#include "net/proto2/util/converter/internal/type_info.h"
#include "net/proto2/util/converter/public/datapiece.h"
#include "net/proto2/util/converter/public/object_writer.h"
#include "net/proto2/util/converter/public/utility.h"
#include "net/proto2/util/public/type_resolver.h"
#include "strings/stringpiece.h"

namespace proto2 {
namespace util {
namespace converter {

// An ObjectWriter that renders non-repeated primitive fields of proto messages
// with their default values. DefaultValueObjectWriter holds objects, lists and
// fields it receives in a tree structure and writes them out to another
// ObjectWriter when EndObject() is called on the root object. It also writes
// out all non-repeated primitive fields that haven't been explicitly rendered
// with their default values (0 for numbers, "" for strings, etc).
class DefaultValueObjectWriter : public ObjectWriter {
 public:
#ifndef PROTO2_OPENSOURCE
  DefaultValueObjectWriter(const TypeInfo& typeinfo,
                           const google::protobuf::Type& type,
                           ObjectWriter* ow);
#endif  // !PROTO2_OPENSOURCE
  DefaultValueObjectWriter(TypeResolver* type_resolver,
                           const google::protobuf::Type& type,
                           ObjectWriter* ow);

  virtual ~DefaultValueObjectWriter();

  // ObjectWriter methods.
  virtual DefaultValueObjectWriter* StartObject(StringPiece name);

  virtual DefaultValueObjectWriter* EndObject();

  virtual DefaultValueObjectWriter* StartList(StringPiece name);

  virtual DefaultValueObjectWriter* EndList();

  virtual DefaultValueObjectWriter* RenderBool(StringPiece name, bool value);

  virtual DefaultValueObjectWriter* RenderInt32(StringPiece name, int32 value);

  virtual DefaultValueObjectWriter* RenderUint32(StringPiece name,
                                                 uint32 value);

  virtual DefaultValueObjectWriter* RenderInt64(StringPiece name, int64 value);

  virtual DefaultValueObjectWriter* RenderUint64(StringPiece name,
                                                 uint64 value);

  virtual DefaultValueObjectWriter* RenderDouble(StringPiece name,
                                                 double value);

  virtual DefaultValueObjectWriter* RenderFloat(StringPiece name, float value);

  virtual DefaultValueObjectWriter* RenderString(StringPiece name,
                                                 StringPiece value);
#ifdef PROTO2_OPENSOURCE
  virtual DefaultValueObjectWriter* RenderBytes(StringPiece name,
                                                StringPiece value);
#else   // PROTO2_OPENSOURCE
  virtual DefaultValueObjectWriter* RenderCord(StringPiece name,
                                               const Cord& value);
#endif  // !PROTO2_OPENSOURCE

  virtual DefaultValueObjectWriter* RenderNull(StringPiece name);

  virtual DefaultValueObjectWriter* DisableCaseNormalizationForNextKey();

 private:
  enum NodeKind {
    PRIMITIVE = 0,
    OBJECT = 1,
    LIST = 2,
    MAP = 3,
  };

  // "Node" represents a node in the tree that holds the input of
  // DefaultValueObjectWriter.
  class Node {
   public:
    Node(const string& name, const google::protobuf::Type* type, NodeKind kind,
         const DataPiece& data, bool is_placeholder);
    virtual ~Node() {
      for (int i = 0; i < children_.size(); ++i) {
        delete children_[i];
      }
    }

    // Adds a child to this node. Takes ownership of this child.
    void AddChild(Node* child) { children_.push_back(child); }

    // Finds the child given its name.
    Node* FindChild(StringPiece name);

    // Populates children of this Node based on its type. If there are already
    // children created, they will be merged to the result. Caller should pass
    // in TypeInfo for looking up types of the children.
    void PopulateChildren(const TypeInfo* typeinfo);

    // If this node is a leaf (has data), writes the current node to the
    // ObjectWriter; if not, then recursively writes the children to the
    // ObjectWriter.
    void WriteTo(ObjectWriter* ow);

    // Accessors
    const string& name() const { return name_; }

    const google::protobuf::Type* type() { return type_; }

    void set_type(const google::protobuf::Type* type) { type_ = type; }

    NodeKind kind() { return kind_; }

    int number_of_children() { return children_.size(); }

    void set_data(const DataPiece& data) { data_ = data; }

    void set_disable_normalize(bool disable_normalize) {
      disable_normalize_ = disable_normalize;
    }

    bool is_any() { return is_any_; }

    void set_is_any(bool is_any) { is_any_ = is_any; }

    void set_is_placeholder(bool is_placeholder) {
      is_placeholder_ = is_placeholder;
    }

   private:
    // Returns the Value Type of a map given the Type of the map entry and a
    // TypeInfo instance.
    const google::protobuf::Type* GetMapValueType(
        const google::protobuf::Type& entry_type, const TypeInfo* typeinfo);

    // Calls WriteTo() on every child in children_.
    void WriteChildren(ObjectWriter* ow);

    // The name of this node.
    string name_;
    // google::protobuf::Type of this node. Owned by TypeInfo.
    const google::protobuf::Type* type_;
    // The kind of this node.
    NodeKind kind_;
    // Whether to disable case normalization of the name.
    bool disable_normalize_;
    // Whether this is a node for "Any".
    bool is_any_;
    // The data of this node when it is a leaf node.
    DataPiece data_;
    // Children of this node.
    std::vector<Node*> children_;
    // Whether this node is a placeholder for an object or list automatically
    // generated when creating the parent node. Should be set to false after
    // the parent node's StartObject()/StartList() method is called with this
    // node's name.
    bool is_placeholder_;
  };

  // Populates children of "node" if it is an "any" Node and its real type has
  // been given.
  void MaybePopulateChildrenOfAny(Node* node);

  // Writes the root_ node to ow_ and resets the root_ and current_ pointer to
  // nullptr.
  void WriteRoot();

  // Creates a DataPiece containing the default value of the type of the field.
  static DataPiece CreateDefaultDataPieceForField(
      const google::protobuf::Field& field);

  // Returns disable_normalize_ and reset it to false.
  bool GetAndResetDisableNormalize() {
    return disable_normalize_ ? (disable_normalize_ = false, true) : false;
  }

  // Adds or replaces the data_ of a primitive child node.
  void RenderDataPiece(StringPiece name, const DataPiece& data);

  // Type information for all the types used in the descriptor. Used to find
  // google::protobuf::Type of nested messages/enums.
  const TypeInfo* typeinfo_;
  // Whether the TypeInfo object is owned by this class.
  bool own_typeinfo_;
  // google::protobuf::Type of the root message type.
  const google::protobuf::Type& type_;
  // Holds copies of strings passed to RenderString.
  vector<string*> string_values_;

  // Whether to disable case normalization of the next node.
  bool disable_normalize_;
  // The current Node. Owned by its parents.
  Node* current_;
  // The root Node.
  std::unique_ptr<Node> root_;
  // The stack to hold the path of Nodes from current_ to root_;
  std::stack<Node*> stack_;

  ObjectWriter* ow_;

  DISALLOW_COPY_AND_ASSIGN(DefaultValueObjectWriter);
};

}  // namespace converter
}  // namespace util
}  // namespace proto2

#endif  // NET_PROTO2_UTIL_CONVERTER_INTERNAL_DEFAULT_VALUE_OBJECTWRITER_H_
