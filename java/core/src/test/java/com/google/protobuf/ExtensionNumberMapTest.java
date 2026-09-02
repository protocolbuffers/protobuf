// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import java.util.Arrays;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class ExtensionNumberMapTest {

  private static final class Entry {
    private final String name;
    private final int number;

    private Entry(String name, int number) {
      this.name = name;
      this.number = number;
    }

    private String getName() {
      return name;
    }

    private int getNumber() {
      return number;
    }
  }

  private static Entry newEntry(int number) {
    return new Entry("foo.Bar" + number, number);
  }

  @Test
  public void empty() {
    ExtensionNumberMap<Entry, String> map =
        new ExtensionNumberMap<>(Entry::getName, Entry::getNumber);
    assertThat(map).isEmpty();
    assertThat(map).hasSize(0);
    assertThat(map.get("foo.Bar", 1)).isNull();
    assertThat(map.containsKey("foo.Bar", 1)).isFalse();
    assertThat(map.remove("foo.Bar", 1)).isNull();
  }

  @Test
  public void addRemove() {
    ExtensionNumberMap<Entry, String> map =
        new ExtensionNumberMap<>(Entry::getName, Entry::getNumber);
    Entry entry = newEntry(1);
    assertThat(map.put(entry)).isNull();
    assertThat(map).isNotEmpty();
    assertThat(map).hasSize(1);
    assertThat(map.get("foo.Bar1", 1)).isEqualTo(entry);
    assertThat(map.containsKey("foo.Bar1", 1)).isTrue();
    assertThat(map).containsExactly(entry);

    assertThat(map.remove(entry)).isEqualTo(entry);
    assertThat(map).isEmpty();
    assertThat(map).hasSize(0);
  }

  @Test
  public void insertReplace() {
    ExtensionNumberMap<Entry, String> map =
        new ExtensionNumberMap<>(Entry::getName, Entry::getNumber);
    Entry entry = newEntry(1);
    assertThat(map.put(entry)).isNull();
    Entry dupEntry = newEntry(1);
    assertThat(map.put(dupEntry)).isSameInstanceAs(entry);
  }

  @Test
  public void stress() {
    ExtensionNumberMap<Entry, String> map =
        new ExtensionNumberMap<>(Entry::getName, Entry::getNumber);
    Entry[] entries = new Entry[64];
    for (int i = 0; i < entries.length; ++i) {
      entries[i] = newEntry(i + 1);
    }
    // Add.
    for (int i = 0; i < entries.length; ++i) {
      Entry entry = entries[i];
      assertThat(map.put(entry)).isNull();
      assertThat(map).isNotEmpty();
      assertThat(map).hasSize(i + 1);
      for (int j = 0; j <= i; ++j) {
        Entry prevEntry = entries[j];
        assertThat(map.containsKey(prevEntry.getName(), prevEntry.getNumber())).isTrue();
        assertThat(map.get(prevEntry.getName(), prevEntry.getNumber())).isEqualTo(prevEntry);
      }
      assertThat(map).containsExactlyElementsIn(Arrays.copyOfRange(entries, 0, i + 1));
    }
    // Remove.
    for (int i = entries.length; i > 0; --i) {
      Entry entry = entries[i - 1];
      assertThat(map).isNotEmpty();
      assertThat(map).hasSize(i);
      assertThat(map.remove(entry.getName(), entry.getNumber())).isEqualTo(entry);
      assertThat(map).containsExactlyElementsIn(Arrays.copyOfRange(entries, 0, i - 1));
    }
  }

  private static final class ThrowingEntry {
    private final String name;
    private final int number;
    private boolean throwOnGet;

    private ThrowingEntry(String name, int number) {
      this.name = name;
      this.number = number;
    }

    private String getName() {
      if (throwOnGet) {
        throw new RuntimeException("Simulated exception during resize");
      }
      return name;
    }

    private int getNumber() {
      return number;
    }
  }

  @Test
  public void resizeFailure() {
    ExtensionNumberMap<ThrowingEntry, String> map =
        new ExtensionNumberMap<>(ThrowingEntry::getName, ThrowingEntry::getNumber);

    ThrowingEntry[] entries = new ThrowingEntry[9];
    for (int i = 0; i < 9; i++) {
      entries[i] = new ThrowingEntry("foo.Bar" + (i + 1), i + 1);
      map.put(entries[i]);
    }

    assertThat(map).hasSize(9);

    // Now set throwOnGet = true for one of the elements
    entries[4].throwOnGet = true;

    ThrowingEntry tenthEntry = new ThrowingEntry("foo.Bar10", 10);
    RuntimeException expected = assertThrows(RuntimeException.class, () -> map.put(tenthEntry));
    assertThat(expected).hasMessageThat().contains("Simulated exception during resize");

    // Reset throwOnGet to false to verify map contents.
    entries[4].throwOnGet = false;

    // Verify that map size remains 9 and we did not include the 10th element.
    assertThat(map).hasSize(9);
    for (int i = 0; i < 9; i++) {
      assertThat(map.get("foo.Bar" + (i + 1), i + 1)).isEqualTo(entries[i]);
    }

    // Let's make sure the 10th element was cleared and is not retrieved.
    assertThat(map.get("foo.Bar10", 10)).isNull();
  }
}
