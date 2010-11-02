// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

import protobuf_unittest.Vehicle;
import protobuf_unittest.Wheel;

import junit.framework.TestCase;

import java.util.List;
import java.util.ArrayList;

/**
 * Test cases that exercise end-to-end use cases involving
 * {@link SingleFieldBuilder} and {@link RepeatedFieldBuilder}.
 *
 * @author jonp@google.com (Jon Perlow)
 */
public class NestedBuildersTest extends TestCase {

  public void testMessagesAndBuilders() {
    Vehicle.Builder vehicleBuilder = Vehicle.newBuilder();
    vehicleBuilder.addWheelBuilder()
        .setRadius(4)
        .setWidth(1);
    vehicleBuilder.addWheelBuilder()
        .setRadius(4)
        .setWidth(2);
    vehicleBuilder.addWheelBuilder()
        .setRadius(4)
        .setWidth(3);
    vehicleBuilder.addWheelBuilder()
        .setRadius(4)
        .setWidth(4);
    vehicleBuilder.getEngineBuilder()
        .setLiters(10);

    Vehicle vehicle = vehicleBuilder.build();
    assertEquals(4, vehicle.getWheelCount());
    for (int i = 0; i < 4; i++) {
      Wheel wheel = vehicle.getWheel(i);
      assertEquals(4, wheel.getRadius());
      assertEquals(i + 1, wheel.getWidth());
    }
    assertEquals(10, vehicle.getEngine().getLiters());

    for (int i = 0; i < 4; i++) {
      vehicleBuilder.getWheelBuilder(i)
          .setRadius(5)
          .setWidth(i + 10);
    }
    vehicleBuilder.getEngineBuilder().setLiters(20);

    vehicle = vehicleBuilder.build();
    for (int i = 0; i < 4; i++) {
      Wheel wheel = vehicle.getWheel(i);
      assertEquals(5, wheel.getRadius());
      assertEquals(i + 10, wheel.getWidth());
    }
    assertEquals(20, vehicle.getEngine().getLiters());
    assertTrue(vehicle.hasEngine());
  }

  public void testMessagesAreCached() {
    Vehicle.Builder vehicleBuilder = Vehicle.newBuilder();
    vehicleBuilder.addWheelBuilder()
        .setRadius(1)
        .setWidth(2);
    vehicleBuilder.addWheelBuilder()
        .setRadius(3)
        .setWidth(4);
    vehicleBuilder.addWheelBuilder()
        .setRadius(5)
        .setWidth(6);
    vehicleBuilder.addWheelBuilder()
        .setRadius(7)
        .setWidth(8);

    // Make sure messages are cached.
    List<Wheel> wheels = new ArrayList<Wheel>(vehicleBuilder.getWheelList());
    for (int i = 0; i < wheels.size(); i++) {
      assertSame(wheels.get(i), vehicleBuilder.getWheel(i));
    }

    // Now get builders and check they didn't change.
    for (int i = 0; i < wheels.size(); i++) {
      vehicleBuilder.getWheel(i);
    }
    for (int i = 0; i < wheels.size(); i++) {
      assertSame(wheels.get(i), vehicleBuilder.getWheel(i));
    }

    // Change just one
    vehicleBuilder.getWheelBuilder(3)
        .setRadius(20).setWidth(20);

    // Now get wheels and check that only that one changed
    for (int i = 0; i < wheels.size(); i++) {
      if (i < 3) {
        assertSame(wheels.get(i), vehicleBuilder.getWheel(i));
      } else {
        assertNotSame(wheels.get(i), vehicleBuilder.getWheel(i));
      }
    }
  }

  public void testRemove_WithNestedBuilders() {
    Vehicle.Builder vehicleBuilder = Vehicle.newBuilder();
    vehicleBuilder.addWheelBuilder()
        .setRadius(1)
        .setWidth(1);
    vehicleBuilder.addWheelBuilder()
        .setRadius(2)
        .setWidth(2);
    vehicleBuilder.removeWheel(0);

    assertEquals(1, vehicleBuilder.getWheelCount());
    assertEquals(2, vehicleBuilder.getWheel(0).getRadius());
  }

  public void testRemove_WithNestedMessages() {
    Vehicle.Builder vehicleBuilder = Vehicle.newBuilder();
    vehicleBuilder.addWheel(Wheel.newBuilder()
        .setRadius(1)
        .setWidth(1));
    vehicleBuilder.addWheel(Wheel.newBuilder()
        .setRadius(2)
        .setWidth(2));
    vehicleBuilder.removeWheel(0);

    assertEquals(1, vehicleBuilder.getWheelCount());
    assertEquals(2, vehicleBuilder.getWheel(0).getRadius());
  }

  public void testMerge() {
    Vehicle vehicle1 = Vehicle.newBuilder()
        .addWheel(Wheel.newBuilder().setRadius(1).build())
        .addWheel(Wheel.newBuilder().setRadius(2).build())
        .build();

    Vehicle vehicle2 = Vehicle.newBuilder()
        .mergeFrom(vehicle1)
        .build();
    // List should be the same -- no allocation
    assertSame(vehicle1.getWheelList(), vehicle2.getWheelList());

    Vehicle vehicle3 = vehicle1.toBuilder().build();
    assertSame(vehicle1.getWheelList(), vehicle3.getWheelList());
  }

  public void testGettingBuilderMarksFieldAsHaving() {
    Vehicle.Builder vehicleBuilder = Vehicle.newBuilder();
    vehicleBuilder.getEngineBuilder();
    Vehicle vehicle = vehicleBuilder.buildPartial();
    assertTrue(vehicle.hasEngine());
  }
}
