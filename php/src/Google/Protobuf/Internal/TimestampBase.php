<?php

namespace Google\Protobuf\Internal;

/**
 * Base class for Google\Protobuf\Timestamp, this contains hand-written
 * convenience methods.
 */
class TimestampBase extends \Google\Protobuf\Internal\Message
{
    /*
     * Converts PHP DateTime to Timestamp.
     *
     * @param \DateTime $datetime
     */
    public function fromDateTime(\DateTime $datetime)
    {
        $this->seconds = $datetime->getTimestamp();
        $this->nanos = 1000 * $datetime->format('u');
    }

    /**
     * Converts Timestamp to PHP DateTime.
     *
     * @return \DateTime $datetime
     */
    public function toDateTime()
    {
        if ($this->nanos < 0 || $this->nanos > 999999999) {
            throw new \Exception("Nanoseconds must be in the range of 0 to 999,999,999 nanoseconds.");
        }
        $time = sprintf('%s.%06d', $this->seconds, $this->nanos / 1000);
        $ret = \DateTime::createFromFormat('U.u', $time);
        if ($ret === false) {
            throw new \Exception("Cannot create DateTime.");
        }
        return $ret;
    }
}
