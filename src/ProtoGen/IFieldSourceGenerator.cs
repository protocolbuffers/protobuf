namespace Google.ProtocolBuffers.ProtoGen {
  internal interface IFieldSourceGenerator {
    void GenerateMembers(TextGenerator writer);
    void GenerateBuilderMembers(TextGenerator writer);
    void GenerateMergingCode(TextGenerator writer);
    void GenerateBuildingCode(TextGenerator writer);
    void GenerateParsingCode(TextGenerator writer);
    void GenerateSerializationCode(TextGenerator writer);
    void GenerateSerializedSizeCode(TextGenerator writer);
  }
}
