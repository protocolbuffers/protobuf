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
     * Nanos must be in [0, 999999999] per timestamp.proto. Seconds should be
     * in [-62135596800, 253402300799] (0001-01-01T00:00:00Z through
     * 9999-12-31T23:59:59Z); values outside that range may fail conversion.
     *
     * @return \DateTime $datetime
     * @throws \UnexpectedValueException if the Timestamp cannot be converted
     */
    public function toDateTime()
    {
        if ($this->nanos < 0 || $this->nanos > 999999999) {
            throw new \UnexpectedValueException(
                'Timestamp nanos out of range: must be in [0, 999999999]'
            );
        }

        $time = sprintf('%s.%06d', $this->seconds, intdiv($this->nanos, 1000));
        $datetime = \DateTime::createFromFormat('U.u', $time);
        if ($datetime === false) {
            throw new \UnexpectedValueException(
                'Cannot create DateTime from Timestamp.'
            );
        }
        return $datetime;
    }
}
