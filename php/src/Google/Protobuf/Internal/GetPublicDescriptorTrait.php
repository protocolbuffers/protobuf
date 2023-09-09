<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf\Internal;

trait GetPublicDescriptorTrait
{
    private function getPublicDescriptor($desc)
    {
        return is_null($desc) ? null : $desc->getPublicDescriptor();
    }
}
