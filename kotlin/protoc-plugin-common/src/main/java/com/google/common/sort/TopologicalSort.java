/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.common.sort;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.PriorityQueue;

/**
 * Topological sorting. The algorithm is an adaptation of the topological sorting algorithm
 * described in TAOCP Section 2.2.3, with a little bit of extra state to provide a stable sort. The
 * constructed ordering is guaranteed to be deterministic.
 *
 * <p>The elements to be sorted should implement the standard {@link Object#hashCode} and {@link
 * Object#equals} methods.
 */
public final class TopologicalSort {
  /**
   * This exception is thrown whenever the input to our topological sort algorithm is a cyclical
   * graph, or the predecessor relation refers to elements that have not been presented as input to
   * the topological sort.
   */
  public static class CyclicalGraphException extends RuntimeException {
    private static final long serialVersionUID = 1L;
    // not parameterized because exceptions can't be parameterized
    private final List<?> elementsInCycle;

    private CyclicalGraphException(String message, List<?> elementsInCycle) {
      super(message);
      this.elementsInCycle = elementsInCycle;
    }

    /**
     * Returns a list of the elements that are part of the cycle, as well as elements that are
     * greater than the elements in the cycle, according to the partial ordering. The elements in
     * this list are not in a meaningful order.
     */
    @SuppressWarnings("unchecked")
    public <T> List<T> getElementsInCycle() {
      return (List<T>) Collections.unmodifiableList(elementsInCycle);
    }
  }

  /**
   * To bundle an element with a mutable structure of the dependency graph.
   *
   * <p>Each {@link InternalElement} counts how many predecessors it has left. Rather than keep a
   * list of predecessors, we reverse the relation so that it's easy to navigate to the successors
   * when an {@link InternalElement} is selected for sorting.
   *
   * <p>This maintains a {@code originalIndex} to allow a "stable" sort based on the original
   * position in the input list.
   */
  private static final class InternalElement<T> implements Comparable<InternalElement<T>> {
    T element;
    int originalIndex;
    List<InternalElement<T>> successors;
    int predecessorCount;

    InternalElement(T element, int originalIndex) {
      this.element = element;
      this.originalIndex = originalIndex;
      this.successors = new ArrayList<InternalElement<T>>();
    }

    @Override
    public int compareTo(InternalElement<T> o) {
      if (originalIndex < o.originalIndex) {
        return -1;
      } else if (originalIndex > o.originalIndex) {
        return 1;
      }
      return 0;
    }
  }

  /**
   * Does a topological sorting. If there is more than one possibility the order returned is based
   * on the order of the input list (note there are teams relying on this deterministic behavior).
   * The input list is mutated to reflect the topological sort. Complexity is O(elements.size() *
   * log(elements.size()) + number of dependencies).
   *
   * <p>A high-level sketch of toplogical sort from Wikipedia:
   * (http://en.wikipedia.org/wiki/Topological_sorting) <code>
   * L ← Empty list that will contain the sorted elements
   * S ← Set of all nodes with no incoming edges
   * while S is non-empty do
   *     remove a node n from S
   *     add n to tail of L
   *     for each node m with an edge e from n to m do
   *         remove edge e from the graph
   *         if m has no other incoming edges then
   *             insert m into S
   * if graph has edges then
   *     return error (graph has at least one cycle)
   * else
   *     return L (a topologically sorted order)
   * </code>
   *
   * <p>We extend the basic algorithm to traverse {@code S} in a particular order based on the
   * original order of the elements to enforce a deterministic result.
   *
   * @param elements a mutable list of elements to be sorted.
   * @param order the partial order between elements.
   * @throws CyclicalGraphException if the graph is cyclical or any predecessor is not present in
   *     the input list.
   */
  public static <T> void sort(List<T> elements, PartialOrdering<T> order) {
    List<InternalElement<T>> internalElements = internalizeElements(elements, order);
    List<InternalElement<T>> sortedElements = new ArrayList<InternalElement<T>>(elements.size());

    // The "S" set of the above algorithm pseudocode is represented here by both
    // readyElementsForCurrentPass and readyElementsForNextPass.
    PriorityQueue<InternalElement<T>> readyElementsForCurrentPass =
        new PriorityQueue<InternalElement<T>>();
    List<InternalElement<T>> readyElementsForNextPass = new ArrayList<InternalElement<T>>();
    for (InternalElement<T> element : internalElements) {
      if (element.predecessorCount == 0) {
        readyElementsForNextPass.add(element);
      }
    }

    while (!readyElementsForNextPass.isEmpty()) {
      readyElementsForCurrentPass.addAll(readyElementsForNextPass);
      readyElementsForNextPass.clear();

      while (!readyElementsForCurrentPass.isEmpty()) {
        InternalElement<T> currentElement = readyElementsForCurrentPass.poll();
        sortedElements.add(currentElement);
        for (InternalElement<T> successor : currentElement.successors) {
          successor.predecessorCount--;
          if (successor.predecessorCount == 0) {
            // Subtle: the original algorithm processed the input element list in multiple passes.
            // For each pass, it iterated across the unsorted portion, and added nodes with
            // no dependencies to the sorted portion, maintaining the original order of the
            // unsorted portion. Once the pass hit the end of the list, it went to the next pass,
            // starting at the beginning of the unsorted portion again.
            //
            // Existing Google code requires this ordering behavior, and to preserve it,
            // we queue up some the elements that would otherwise be directly added to
            // readyElementsForCurrentPass, and put those elements into
            // readyElementsForNextPass.
            if (successor.originalIndex < currentElement.originalIndex) {
              readyElementsForNextPass.add(successor);
            } else {
              readyElementsForCurrentPass.add(successor);
            }
          }
        }
      }
    }

    if (sortedElements.size() != elements.size()) {
      List<T> elementsInCycle = new ArrayList<T>();
      for (InternalElement<T> element : internalElements) {
        if (element.predecessorCount > 0) {
          elementsInCycle.add(element.element);
        }
      }
      throw new CyclicalGraphException(
          "Cyclical graphs can not be topologically sorted.", elementsInCycle);
    }

    overwriteListWithSortedResults(elements, sortedElements);
  }

  /**
   * Does a topological sorting. If there is a tie in the topographical ordering, it is resolved in
   * favor of the order of the original input list. Thus, an arbitrary "lexicographical topological
   * ordering" can be achieved by first lexicographically sorting the input list according to your
   * own criteria and then calling this method.
   *
   * <p>A high-level sketch of toplogical sort from Wikipedia:
   * (http://en.wikipedia.org/wiki/Topological_sorting) <code>
   * L ← Empty list that will contain the sorted elements
   * S ← Set of all nodes with no incoming edges
   * while S is non-empty do
   *     remove a node n from S
   *     add n to tail of L
   *     for each node m with an edge e from n to m do
   *         remove edge e from the graph
   *         if m has no other incoming edges then
   *             insert m into S
   * if graph has edges then
   *     return error (graph has at least one cycle)
   * else
   *     return L (a topologically sorted order)
   * </code>
   *
   * <p>We extend the basic algorithm to traverse {@code S} in a particular order based on the
   * original order of the elements to enforce a deterministic result (lexicographically based on
   * the order of elements in the original input list).
   *
   * @param elements a mutable list of elements to be sorted.
   * @param order the partial order between elements.
   * @throws CyclicalGraphException if the graph is cyclical or any predecessor is not present in
   *     the input list.
   */
  public static <T> void sortLexicographicallyLeast(List<T> elements, PartialOrdering<T> order) {
    List<InternalElement<T>> internalElements = internalizeElements(elements, order);
    List<InternalElement<T>> sortedElements = new ArrayList<InternalElement<T>>(elements.size());

    // The "S" set of the above algorithm pseudocode is represented here by readyElements.
    PriorityQueue<InternalElement<T>> readyElements = new PriorityQueue<InternalElement<T>>();
    for (InternalElement<T> element : internalElements) {
      if (element.predecessorCount == 0) {
        readyElements.add(element);
      }
    }

    while (!readyElements.isEmpty()) {
      InternalElement<T> currentElement = readyElements.poll();
      sortedElements.add(currentElement);
      for (InternalElement<T> successor : currentElement.successors) {
        successor.predecessorCount--;
        if (successor.predecessorCount == 0) {
          readyElements.add(successor);
        }
      }
    }

    if (sortedElements.size() != elements.size()) {
      List<T> elementsInCycle = new ArrayList<T>();
      for (InternalElement<T> element : internalElements) {
        if (element.predecessorCount > 0) {
          elementsInCycle.add(element.element);
        }
      }
      throw new CyclicalGraphException(
          "Cyclical graphs can not be topologically sorted.", elementsInCycle);
    }

    overwriteListWithSortedResults(elements, sortedElements);
  }

  /**
   * Internalizes the elements of the input list, representing the dependency structure to make
   * topological sort easier to compute.
   *
   * @param elements the list to be sorted.
   * @param order the partial ordering used to find the predecessors of each element in the list.
   * @return a list of {@link InternalElement}s initialized with dependency structure.
   */
  private static <T> List<InternalElement<T>> internalizeElements(
      List<T> elements, PartialOrdering<T> order) {
    List<InternalElement<T>> internalElements = new ArrayList<InternalElement<T>>(elements.size());
    // Subtle: due to the potential for duplicates in elements, we need to map every element to a
    // list of the corresponding InternalElements.
    Map<T, List<InternalElement<T>>> internalElementsByValue =
        new HashMap<T, List<InternalElement<T>>>(elements.size());
    int index = 0;
    for (T element : elements) {
      InternalElement<T> internalElement = new InternalElement<T>(element, index);
      internalElements.add(internalElement);
      List<InternalElement<T>> lst = internalElementsByValue.get(element);
      if (lst == null) {
        lst = new ArrayList<InternalElement<T>>();
        internalElementsByValue.put(element, lst);
      }
      lst.add(internalElement);
      index++;
    }

    for (InternalElement<T> internalElement : internalElements) {
      for (T predecessor : order.getPredecessors(internalElement.element)) {
        List<InternalElement<T>> internalPredecessors = internalElementsByValue.get(predecessor);
        if (internalPredecessors != null) {
          for (InternalElement<T> internalPredecessor : internalPredecessors) {
            internalPredecessor.successors.add(internalElement);
            internalElement.predecessorCount++;
          }
        } else {
          // Subtle: we must leave the predecessor count incremented here to properly
          // be able to report CyclicGraphExceptions. In this case, a predecessor was
          // reported by the order relation, but the predecessor was not a member of
          // elements.
          internalElement.predecessorCount++;
        }
      }
    }
    return internalElements;
  }

  /**
   * To copy the sorted elements back into the input list.
   *
   * @param elements the list that is being sorted.
   * @param sortedResults the internal list that holds the sorted results.
   */
  private static <T> void overwriteListWithSortedResults(
      List<T> elements, List<InternalElement<T>> sortedResults) {
    elements.clear();
    for (InternalElement<T> internalElement : sortedResults) {
      elements.add(internalElement.element);
    }
  }
}

