<?php
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# NO CHECKED-IN PROTOBUF GENCODE
# source: google/protobuf/descriptor.proto

namespace Google\Protobuf\Internal;

use Google\Protobuf\Internal\GPBType;
use Google\Protobuf\Internal\GPBWire;
use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\Internal\InputStream;
use Google\Protobuf\Internal\GPBUtil;

/**
 * Generated from protobuf message <code>google.protobuf.ExtensionRangeOptions</code>
 */
class ExtensionRangeOptions extends \Google\Protobuf\Internal\Message
{
    /**
     * The parser stores options it doesn't recognize here. See above.
     *
     * Generated from protobuf field <code>repeated .google.protobuf.UninterpretedOption uninterpreted_option = 999;</code>
     */
    private $uninterpreted_option;
    /**
     * For external users: DO NOT USE. We are in the process of open sourcing
     * extension declaration and executing internal cleanups before it can be
     * used externally.
     *
     * Generated from protobuf field <code>repeated .google.protobuf.ExtensionRangeOptions.Declaration declaration = 2 [retention = RETENTION_SOURCE];</code>
     */
    private $declaration;
    /**
     * Any features defined in the specific edition.
     *
     * Generated from protobuf field <code>optional .google.protobuf.FeatureSet features = 50;</code>
     */
    protected $features = null;
    /**
     * The verification state of the range.
     * TODO: flip the default to DECLARATION once all empty ranges
     * are marked as UNVERIFIED.
     *
     * Generated from protobuf field <code>optional .google.protobuf.ExtensionRangeOptions.VerificationState verification = 3 [default = UNVERIFIED, retention = RETENTION_SOURCE];</code>
     */
    protected $verification = null;

    /**
     * Constructor.
     *
     * @param array $data {
     *     Optional. Data for populating the Message object.
     *
     *     @type array<\Google\Protobuf\Internal\UninterpretedOption>|\Google\Protobuf\Internal\RepeatedField $uninterpreted_option
     *           The parser stores options it doesn't recognize here. See above.
     *     @type array<\Google\Protobuf\Internal\ExtensionRangeOptions\Declaration>|\Google\Protobuf\Internal\RepeatedField $declaration
     *           For external users: DO NOT USE. We are in the process of open sourcing
     *           extension declaration and executing internal cleanups before it can be
     *           used externally.
     *     @type \Google\Protobuf\Internal\FeatureSet $features
     *           Any features defined in the specific edition.
     *     @type int $verification
     *           The verification state of the range.
     *           TODO: flip the default to DECLARATION once all empty ranges
     *           are marked as UNVERIFIED.
     * }
     */
    public function __construct($data = NULL) {
        \GPBMetadata\Google\Protobuf\Internal\Descriptor::initOnce();
        parent::__construct($data);
    }

    /**
     * The parser stores options it doesn't recognize here. See above.
     *
     * Generated from protobuf field <code>repeated .google.protobuf.UninterpretedOption uninterpreted_option = 999;</code>
     * @return \Google\Protobuf\Internal\RepeatedField
     */
    public function getUninterpretedOption()
    {
        return $this->uninterpreted_option;
    }

    /**
     * The parser stores options it doesn't recognize here. See above.
     *
     * Generated from protobuf field <code>repeated .google.protobuf.UninterpretedOption uninterpreted_option = 999;</code>
     * @param array<\Google\Protobuf\Internal\UninterpretedOption>|\Google\Protobuf\Internal\RepeatedField $var
     * @return $this
     */
    public function setUninterpretedOption($var)
    {
        $arr = GPBUtil::checkRepeatedField($var, \Google\Protobuf\Internal\GPBType::MESSAGE, \Google\Protobuf\Internal\UninterpretedOption::class);
        $this->uninterpreted_option = $arr;

        return $this;
    }

    /**
     * For external users: DO NOT USE. We are in the process of open sourcing
     * extension declaration and executing internal cleanups before it can be
     * used externally.
     *
     * Generated from protobuf field <code>repeated .google.protobuf.ExtensionRangeOptions.Declaration declaration = 2 [retention = RETENTION_SOURCE];</code>
     * @return \Google\Protobuf\Internal\RepeatedField
     */
    public function getDeclaration()
    {
        return $this->declaration;
    }

    /**
     * For external users: DO NOT USE. We are in the process of open sourcing
     * extension declaration and executing internal cleanups before it can be
     * used externally.
     *
     * Generated from protobuf field <code>repeated .google.protobuf.ExtensionRangeOptions.Declaration declaration = 2 [retention = RETENTION_SOURCE];</code>
     * @param array<\Google\Protobuf\Internal\ExtensionRangeOptions\Declaration>|\Google\Protobuf\Internal\RepeatedField $var
     * @return $this
     */
    public function setDeclaration($var)
    {
        $arr = GPBUtil::checkRepeatedField($var, \Google\Protobuf\Internal\GPBType::MESSAGE, \Google\Protobuf\Internal\ExtensionRangeOptions\Declaration::class);
        $this->declaration = $arr;

        return $this;
    }

    /**
     * Any features defined in the specific edition.
     *
     * Generated from protobuf field <code>optional .google.protobuf.FeatureSet features = 50;</code>
     * @return \Google\Protobuf\Internal\FeatureSet|null
     */
    public function getFeatures()
    {
        return $this->features;
    }

    public function hasFeatures()
    {
        return isset($this->features);
    }

    public function clearFeatures()
    {
        unset($this->features);
    }

    /**
     * Any features defined in the specific edition.
     *
     * Generated from protobuf field <code>optional .google.protobuf.FeatureSet features = 50;</code>
     * @param \Google\Protobuf\Internal\FeatureSet $var
     * @return $this
     */
    public function setFeatures($var)
    {
        GPBUtil::checkMessage($var, \Google\Protobuf\Internal\FeatureSet::class);
        $this->features = $var;

        return $this;
    }

    /**
     * The verification state of the range.
     * TODO: flip the default to DECLARATION once all empty ranges
     * are marked as UNVERIFIED.
     *
     * Generated from protobuf field <code>optional .google.protobuf.ExtensionRangeOptions.VerificationState verification = 3 [default = UNVERIFIED, retention = RETENTION_SOURCE];</code>
     * @return int
     */
    public function getVerification()
    {
        return isset($this->verification) ? $this->verification : 0;
    }

    public function hasVerification()
    {
        return isset($this->verification);
    }

    public function clearVerification()
    {
        unset($this->verification);
    }

    /**
     * The verification state of the range.
     * TODO: flip the default to DECLARATION once all empty ranges
     * are marked as UNVERIFIED.
     *
     * Generated from protobuf field <code>optional .google.protobuf.ExtensionRangeOptions.VerificationState verification = 3 [default = UNVERIFIED, retention = RETENTION_SOURCE];</code>
     * @param int $var
     * @return $this
     */
    public function setVerification($var)
    {
        GPBUtil::checkEnum($var, \Google\Protobuf\Internal\ExtensionRangeOptions\VerificationState::class);
        $this->verification = $var;

        return $this;
    }

}

