<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf\Internal;

use Google\Protobuf\Internal\Edition;
use Google\Protobuf\Internal\FeatureSet;
use Google\Protobuf\Internal\FeatureSet\FieldPresence;
use Google\Protobuf\Internal\FeatureSet\RepeatedFieldEncoding;

class FileDescriptor
{

    private $package;
    private $message_type = [];
    private $enum_type = [];

    public function setPackage($package)
    {
        $this->package = $package;
    }

    public function getPackage()
    {
        return $this->package;
    }

    public function getMessageType()
    {
        return $this->message_type;
    }

    public function addMessageType($desc)
    {
        $this->message_type[] = $desc;
    }

    public function getEnumType()
    {
        return $this->enum_type;
    }

    public function addEnumType($desc)
    {
        $this->enum_type[]= $desc;
    }

    public static function resolveEdition($proto): int
    {
        $syntax = $proto->getSyntax();
        if ($syntax === 'editions') {
            if (!$proto->hasEdition()) {
                throw new \Exception(sprintf(
                    'File "%s" has syntax="editions", but no edition is specified',
                    $proto->getName()));
            }
            return $proto->getEdition();
        }
        if ($syntax === 'proto3') {
            return Edition::EDITION_PROTO3;
        }
        if ($syntax === '' || $syntax === 'proto2') {
            return Edition::EDITION_PROTO2;
        }
        throw new \Exception(sprintf(
            'File "%s" has invalid syntax "%s"', $proto->getName(), $syntax));
    }

    public static function defaultFeatures(int $edition): FeatureSet
    {
        $features = new FeatureSet();
        $features->setFieldPresence(
            $edition === Edition::EDITION_PROTO3
                ? FieldPresence::IMPLICIT
                : FieldPresence::EXPLICIT
        );
        $features->setRepeatedFieldEncoding(
            $edition < Edition::EDITION_PROTO3
                ? RepeatedFieldEncoding::EXPANDED
                : RepeatedFieldEncoding::PACKED
        );
        return $features;
    }

    public static function resolveFeatures(
        FeatureSet $parent,
        ?FeatureSet $child
    ): FeatureSet {
        if ($child === null) {
            return $parent;
        }
        $resolved = new FeatureSet();
        $resolved->mergeFrom($parent);
        $resolved->mergeFrom($child);
        return $resolved;
    }

    public static function fileFeatures($proto, int $edition): FeatureSet
    {
        $options = $proto->getOptions();
        return self::resolveFeatures(
            self::defaultFeatures($edition),
            $options !== null ? $options->getFeatures() : null
        );
    }

    public static function buildFromProto($proto, $custom_json_names = [])
    {
        $file = new FileDescriptor();
        $file->setPackage($proto->getPackage());
        // Reject invalid syntax even if the file declares no messages.
        self::resolveEdition($proto);
        foreach ($proto->getMessageType() as $message_proto) {
            $file->addMessageType(Descriptor::buildFromProto(
                $message_proto, $proto, "", $custom_json_names));
        }
        foreach ($proto->getEnumType() as $enum_proto) {
            $file->addEnumType(
                EnumDescriptor::buildFromProto(
                    $enum_proto,
                    $proto,
                    "",
                    $custom_json_names));
        }
        return $file;
    }
}
