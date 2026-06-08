package com.google.protobuf;

final class FieldInfo implements Comparable<FieldInfo> {
  private final int fieldNumber_;

  FieldInfo(int fieldNumber) {
    this.fieldNumber_ = fieldNumber;
  }

  public int getFieldNumber() {
    return fieldNumber_;
  }

  @Override
  public int compareTo(FieldInfo o) {
    return Integer.compare(fieldNumber_, o.fieldNumber_);
  }
}
