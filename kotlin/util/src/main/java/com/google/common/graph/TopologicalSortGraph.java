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

package com.google.common.graph;

import static com.google.common.base.Preconditions.checkArgument;
import com.google.common.sort.TopologicalSort;
import com.google.common.sort.PartialOrdering;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Set;

public final class TopologicalSortGraph
{
  public static <N> List<N> topologicalOrdering(final Graph<N> graph) {
    checkArgument(graph.isDirected(), "Cannot get topological ordering of an undirected graph.");
    PartialOrdering<N> partialOrdering =
        new PartialOrdering<N>() {
          @Override
          public Set<N> getPredecessors(N node) {
            return graph.predecessors(node);
          }
        };
    List<N> nodeList = new ArrayList<N>(graph.nodes());
    TopologicalSort.sortLexicographicallyLeast(nodeList, partialOrdering);
    return Collections.unmodifiableList(nodeList);
  }
}