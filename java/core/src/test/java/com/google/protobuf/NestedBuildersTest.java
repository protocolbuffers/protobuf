// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import protobuf_unittest.Engine;
import protobuf_unittest.Vehicle;
import protobuf_unittest.Wheel;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Test cases that exercise end-to-end use cases involving {@link SingleFieldBuilder} and {@link
 * RepeatedFieldBuilder}.
 */
@RunWith(JUnit4.class)
public class NestedBuildersTest {

  @Test
  public void testMessagesAndBuilders() {
    Vehicle.Builder vehicleBuilder = Vehicle.newBuilder();
    vehicleBuilder.addWheelBuilder().setRadius(4).setWidth(1);
    vehicleBuilder.addWheelBuilder().setRadius(4).setWidth(2);
    vehicleBuilder.addWheelBuilder().setRadius(4).setWidth(3);
    vehicleBuilder.addWheelBuilder().setRadius(4).setWidth(4);
    vehicleBuilder.getEngineBuilder().setLiters(10);

    Vehicle vehicle = vehicleBuilder.build();
    assertThat(vehicle.getWheelCount()).isEqualTo(4);
    for (int i = 0; i < 4; i++) {
      Wheel wheel = vehicle.getWheel(i);
      assertThat(wheel.getRadius()).isEqualTo(4);
      assertThat(wheel.getWidth()).isEqualTo(i + 1);
    }
    assertThat(vehicle.getEngine().getLiters()).isEqualTo(10);

    for (int i = 0; i < 4; i++) {
      vehicleBuilder.getWheelBuilder(i).setRadius(5).setWidth(i + 10);
    }
    Engine.Builder engineBuilder = vehicleBuilder.getEngineBuilder().setLiters(20);

    vehicle = vehicleBuilder.build();
    for (int i = 0; i < 4; i++) {
      Wheel wheel = vehicle.getWheel(i);
      assertThat(wheel.getRadius()).isEqualTo(5);
      assertThat(wheel.getWidth()).isEqualTo(i + 10);
    }
    assertThat(vehicle.getEngine().getLiters()).isEqualTo(20);
    assertThat(vehicle.hasEngine()).isTrue();

    engineBuilder.setLiters(50);
    assertThat(vehicleBuilder.getEngine().getLiters()).isEqualTo(50);
  }

  @Test
  public void testMessagesAreCached() {
    Vehicle.Builder vehicleBuilder = Vehicle.newBuilder();
    vehicleBuilder.addWheelBuilder().setRadius(1).setWidth(2);
    vehicleBuilder.addWheelBuilder().setRadius(3).setWidth(4);
    vehicleBuilder.addWheelBuilder().setRadius(5).setWidth(6);
    vehicleBuilder.addWheelBuilder().setRadius(7).setWidth(8);

    // Make sure messages are cached.
    List<Wheel> wheels = new ArrayList<Wheel>(vehicleBuilder.getWheelList());
    for (int i = 0; i < wheels.size(); i++) {
      assertThat(wheels.get(i)).isSameInstanceAs(vehicleBuilder.getWheel(i));
    }

    // Now get builders and check they didn't change.
    for (int i = 0; i < wheels.size(); i++) {
      vehicleBuilder.getWheel(i);
    }
    for (int i = 0; i < wheels.size(); i++) {
      assertThat(wheels.get(i)).isSameInstanceAs(vehicleBuilder.getWheel(i));
    }

    // Change just one
    vehicleBuilder.getWheelBuilder(3).setRadius(20).setWidth(20);

    // Now get wheels and check that only that one changed
    for (int i = 0; i < wheels.size(); i++) {
      if (i < 3) {
        assertThat(wheels.get(i)).isSameInstanceAs(vehicleBuilder.getWheel(i));
      } else {
        assertThat(wheels.get(i)).isNotSameInstanceAs(vehicleBuilder.getWheel(i));
      }
    }
  }

  @Test
  public void testRemove_WithNestedBuilders() {
    Vehicle.Builder vehicleBuilder = Vehicle.newBuilder();
    vehicleBuilder.addWheelBuilder().setRadius(1).setWidth(1);
    vehicleBuilder.addWheelBuilder().setRadius(2).setWidth(2);
    vehicleBuilder.removeWheel(0);

    assertThat(vehicleBuilder.getWheelCount()).isEqualTo(1);
    assertThat(vehicleBuilder.getWheel(0).getRadius()).isEqualTo(2);
  }

  @Test
  public void testRemove_WithNestedMessages() {
    Vehicle.Builder vehicleBuilder = Vehicle.newBuilder();
    vehicleBuilder.addWheel(Wheel.newBuilder().setRadius(1).setWidth(1));
    vehicleBuilder.addWheel(Wheel.newBuilder().setRadius(2).setWidth(2));
    vehicleBuilder.removeWheel(0);

    assertThat(vehicleBuilder.getWheelCount()).isEqualTo(1);
    assertThat(vehicleBuilder.getWheel(0).getRadius()).isEqualTo(2);
  }

  @Test
  public void testMerge() {
    Vehicle vehicle1 =
        Vehicle.newBuilder()
            .addWheel(Wheel.newBuilder().setRadius(1).build())
            .addWheel(Wheel.newBuilder().setRadius(2).build())
            .build();

    Vehicle vehicle2 = Vehicle.newBuilder().mergeFrom(vehicle1).build();
    // List should be the same -- no allocation
    assertThat(vehicle1.getWheelList()).isSameInstanceAs(vehicle2.getWheelList());

    Vehicle vehicle3 = vehicle1.toBuilder().build();
    assertThat(vehicle1.getWheelList()).isSameInstanceAs(vehicle3.getWheelList());
  }

  @Test
  public void testGettingBuilderMarksFieldAsHaving() {
    Vehicle.Builder vehicleBuilder = Vehicle.newBuilder();
    vehicleBuilder.getEngineBuilder();
    Vehicle vehicle = vehicleBuilder.buildPartial();
    assertThat(vehicle.hasEngine()).isTrue();
  }
}
