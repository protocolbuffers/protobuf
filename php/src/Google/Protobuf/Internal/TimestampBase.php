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
    public function fromDateTime(\DateTimeInterface $datetime): static
    {
        $this->seconds = $datetime->getTimestamp();
        $this->nanos = 1000 * $datetime->format('u');

        return $this;
    }

    /**
     * Converts Timestamp to PHP DateTime.
     *
     * @return \DateTime $datetime
     */
    public function toDateTime()
    {
        $time = sprintf('%s.%06d', $this->seconds, $this->nanos / 1000);
        return \DateTime::createFromFormat('U.u', $time);
    }
}
