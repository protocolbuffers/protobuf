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

import java.util.Set;

/**
 * The interface that imposes a partial order on elements in a DAG to be topologically sorted.
 *
 * @author Okhtay Ilghami (okhtay@google.com)
 */
public interface PartialOrdering<T> {

  /**
   * Returns nodes that are considered "less than" {@code element} for purposes of a {@link
   * TopologicalSort}. Transitive predecessors do not need to be included.
   *
   * <p>For example, if {@code getPredecessors(a)} includes {@code b} and {@code getPredecessors(b)}
   * includes {@code c}, it is not necessary to include {@code c} in {@code getPredecessors(a)}.
   * {@code c} is not a "direct" predecessor of {@code a}.
   */
  Set<? extends T> getPredecessors(T element);
}

