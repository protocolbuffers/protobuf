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
        // A google.protobuf.Timestamp requires 0 <= nanos < 1e9 (see the field
        // documentation in timestamp.proto). Reject out-of-range values, which
        // are reachable from untrusted wire input, rather than formatting an
        // unparseable string and silently returning false.
        if ($this->nanos < 0 || $this->nanos >= 1000000000) {
            throw new \Exception(
                'Cannot convert Timestamp with nanos ' . $this->nanos .
                ' outside [0, 1000000000) to DateTime.');
        }
        $time = sprintf('%s.%06d', $this->seconds, $this->nanos / 1000);
        return \DateTime::createFromFormat('U.u', $time);
    }
}
