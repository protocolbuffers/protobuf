<?php

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

namespace Google\Protobuf\Internal;

function getClassNamePrefix(
    $classname,
    $file_proto)
{
    $option = $file_proto->getOptions();
    $prefix = is_null($option) ? "" : $option->getPhpClassPrefix();
    if ($prefix !== "") {
        return $prefix;
    }

    $reserved_words = array("Empty");
    foreach ($reserved_words as $reserved_word) {
        if ($classname === $reserved_word) {
            if ($file_proto->getPackage() === "google.protobuf") {
                return "GPB";
            } else {
                return "PB";
            }
        }
    }

    return "";
}

function getClassNameWithoutPackage(
    $name,
    $file_proto)
{
    $classname = implode('_', array_map('ucwords', explode('.', $name)));
    return getClassNamePrefix($classname, $file_proto) . $classname;
}

function getFullClassName(
    $proto,
    $containing,
    $file_proto,
    &$message_name_without_package,
    &$classname,
    &$fullname)
{
    // Full name needs to start with '.'.
    $message_name_without_package = $proto->getName();
    if ($containing !== "") {
        $message_name_without_package =
            $containing . "." . $message_name_without_package;
    }

    $package = $file_proto->getPackage();
    if ($package === "") {
        $fullname = "." . $message_name_without_package;
    } else {
        $fullname = "." . $package . "." . $message_name_without_package;
    }

    $class_name_without_package =
        getClassNameWithoutPackage($message_name_without_package, $file_proto);
    if ($package === "") {
        $classname = $class_name_without_package;
    } else {
        $classname =
            implode('\\', array_map('ucwords', explode('.', $package))).
            "\\".$class_name_without_package;
    }
}

function combineInt32ToInt64($high, $low)
{
    $isNeg = $high < 0;
    if ($isNeg) {
        $high = ~$high;
        $low = ~$low;
        $low++;
        if (!$low) {
            $high++;
        }
    }
    $result = bcadd(bcmul($high, 4294967296), $low);
    if ($low < 0) {
        $result = bcadd($result, 4294967296);
    }
    if ($isNeg) {
      $result = bcsub(0, $result);
    }
    return $result;
}
