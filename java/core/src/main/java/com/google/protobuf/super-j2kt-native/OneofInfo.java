package com.google.protobuf;

@SuppressWarnings("nullness")
final class OneofInfo {
  private final int id_;

  public OneofInfo(int id, Object caseField, Object valueField) {
    this.id_ = id;
  }

  public int getId() {
    return id_;
  }

  public Object getCaseField() {
    return null;
  }

  public Object getValueField() {
    return null;
  }
}
