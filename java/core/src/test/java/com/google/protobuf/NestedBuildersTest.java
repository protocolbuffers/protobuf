// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import protobuf_unittest.Engine;
import protobuf_unittest.TimingBelt;
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
  public void test3LayerPropagationWithIntermediateClear() {
    Vehicle.Builder vehicleBuilder = Vehicle.newBuilder();
    vehicleBuilder.getEngineBuilder().getTimingBeltBuilder();

    // This step detaches the TimingBelt.Builder (though it leaves a SingleFieldBuilder in place)
    vehicleBuilder.getEngineBuilder().clear();

    // These steps build the middle and top level messages, it used to leave the vestigial
    // TimingBelt.Builder in a state where further changes didn't propagate anymore
    Object unused = vehicleBuilder.getEngineBuilder().build();
    unused = vehicleBuilder.build();

    TimingBelt expected = TimingBelt.newBuilder().setNumberOfTeeth(124).build();
    vehicleBuilder.getEngineBuilder().setTimingBelt(expected);
    // Testing that b/254158939 is fixed. It used to be that the setTimingBelt call above didn't
    // propagate a change notification and the call below would return a stale version of the timing
    // belt.
    assertThat(vehicleBuilder.getEngine().getTimingBelt()).isEqualTo(expected);
  }

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
