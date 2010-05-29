/*
 * Copyright (C) 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.protobuf;

import java.util.Arrays;

/**
 * A Performance Timing class that can be used to estimate the amount of time a
 * sequence of code takes. The typical code sequence would be as follows:</p>
 * <code>
 PerfTimer pt = new PerfTimer();
 pt.calibrate();
 pt.timeEachAutomatically(new Runnable() = {
    public void run() {
        // Add code to time
    }
 });
 System.out.printf("time per loop=" + pt);

 * The calibrate method determines the overhead of timing the run() method and
 * the number of times to call the run() method to have approximately 1% precision
 * for timing. The method pt.stats() method will return a string containing some
 * statistics tpl, il, ol, min, max, mean, median, stddev and total.
 *
 * tpl    ::= Timer per loop
 * min    ::= minimum time one call to run() took
 * stddev ::= Standard deviation of the collected times
 * mean   ::= the average time to call run()
 * median ::= 1/2 the times were > than this time and 1/2 were less.
 * total  ::= Sum of the times collected.
 * il     ::= innerLoops; the number of times run() between each call to start/stop
 * ol     ::= outerLoops, the number of times start/stop was called
 *
 * You can also use start/stop/restart to do simple timing:
 *
 * pt.start();
 * a += 1;
 * pt.stop();
 * pt.log("time=" + pt);
 * pt.restart();
 * doSomething();
 * pt.stop();
 * System.out.printf("time=" + pt);
 * </code>
 *
 * @author wink@google.com (Wink Saville)
 */
public class PerfTimer {
    /** No debug */
    public static final int DEBUG_LEVEL_NONE = 0;

    /** Some debug */
    public static final int DEBUG_LEVEL_SOME = 1;

    /** All debug */
    public static final int DEBUG_LEVEL_ALL = 2;

    /** Timer ticks per microsecond */
    private static final double TICKS_PER_MICROSECOND = 1000.0;

    /** Random number generator */
    java.util.Random rng = new java.util.Random();

    /** get ticks */
    private static long getTicks() {
        return System.nanoTime();
    }

    /** Debug logging */
    private static void log(String s) {
        System.out.printf(String.format("[PerfTimer] %s\n", s));
    }

    /** Outer loops for timeEachAutomatically */
    private static final int OUTER_LOOPS = 100;

    /** Thrown if an error occurs while timing */
    public static class PerfTimerException extends RuntimeException {
    }

    /**
     * Calibration record
     */
    public static class CalibrationRec {
        /** Runnable overhead */
        public double mRunnableOverheadInMicros = 0.0;

        /** Minimum Threshold value for timeEachAutomaticaly */
        public double mMinThresholdInMicros = 3000.0;

        /** Maximum Threshold value for timeEachAutomaticaly */
        public double mMaxThresholdInMicros = 6000.0;

        /** Desired precision in decimal digits */
        public double mPrecisionInDecimalDigits = 2.0;

        /**
         * Default number of retries if the standard deviation ratio is too
         * large
         */
        public final int mStdDevRetrys = 5;

        /** Default maximum standard deviation radio */
        public final double mMaxStdDevRatio = 0.15;

        /** Number of votes looking for smallest time per loop */
        public final int mVotes = 3;

        /** Convert to string */
        @Override
        public String toString() {
            return String
                    .format(
                            "oh=%.6fus minT=%.6fus maxT=%.6fus prc=%,.3f stdDevRetrys=%d maxStdDevRatio=%.2f votes=%d",
                            mRunnableOverheadInMicros, mMinThresholdInMicros,
                            mMaxThresholdInMicros, mPrecisionInDecimalDigits, mStdDevRetrys,
                            mMaxStdDevRatio, mVotes);
        }
    }

    /**
     * Calibration record
     */
    private CalibrationRec mCr;

    /**
     * Statistics calculated on the timing data.
     */
    public static class Stats {
        /** Number of outer loops */
        private int mOuterLoops;

        /** Number of inner loops */
        private int mInnerLoops;

        /** Minimum time in times array */
        private long mMin;

        /** Maximum time in times array */
        private long mMax;

        /** Median value in times array */
        private double mMedian;

        /** The mean (average) of the values in times array */
        private double mMean;

        /** The standard deviation of the values in times array */
        private double mStdDev;

        private int mStdDevTooLargeCount;

        /** Sum of the times in the times array */
        private double mTotal;

        /** Initialize */
        public void init() {
            mInnerLoops = 1;
            mOuterLoops = 1;
            mMin = 0;
            mMax = 0;
            mMedian = 0;
            mMean = 0;
            mStdDev = 0;
            mStdDevTooLargeCount = 0;
            mTotal = 0;
        }

        /** Constructor */
        public Stats() {
            init();
        }

        /** Set number of inner loops */
        public void setInnerLoops(int loops) {
            mInnerLoops = loops;
        }

        /** Get number of inner loops */
        public int getInnerLoops() {
            return mInnerLoops;
        }

        /** Set number of inner loops */
        public void setOuterLoops(int loops) {
            mOuterLoops = loops;
        }

        /** Get number of inner loops */
        public int getOuterLoops() {
            return mOuterLoops;
        }

        /**
         * Minimum value of collected data in microseconds, valid after analyze.
         */
        public double getMinInMicros() {
            return mMin / TICKS_PER_MICROSECOND;
        }

        /**
         * Maximum value of collected data in microseconds, valid after analyze.
         */
        public double getMaxInMicros() {
            return mMax / TICKS_PER_MICROSECOND;
        }

        /**
         * Sum of the values of collected data in microseconds, valid after
         * analyze.
         */
        public double getTotalInMicros() {
            return mTotal / TICKS_PER_MICROSECOND;
        }

        /** Sum of the values of collected data in seconds, valid after analyze. */
        public double getTotalInSecs() {
            return mTotal / (TICKS_PER_MICROSECOND * 1000000.0);
        }

        /** Sum of the values of collected data in seconds, valid after analyze. */
        public double getMeanInMicros() {
            return mMean / TICKS_PER_MICROSECOND;
        }

        /** Median value of collected data in microseconds, valid after analyze. */
        public double getMedianInMicros() {
            return mMedian / TICKS_PER_MICROSECOND;
        }

        /**
         * Standard deviation of collected data in microseconds, valid after
         * analyze.
         */
        public double getStdDevInMicros() {
            return mStdDev / TICKS_PER_MICROSECOND;
        }

        public double getStdDevRatio() {
            return mStdDev / mMin;
        }

        /** Return true if (mStdDev / mMin) <= maxStdDevRation */
        public boolean stdDevOk(double maxStdDevRatio) {
            return getStdDevRatio() <= maxStdDevRatio;
        }

        /** Increment StdDevTooLargeCount */
        public void incStdDevTooLargeCount() {
            mStdDevTooLargeCount += 1;
        }

        /** Return number of times stdDev was not ok */
        public int getStdDevTooLargeCount() {
            return mStdDevTooLargeCount;
        }

        /** Return time per loop */
        public double getTimePerLoop() {
                return mMin / TICKS_PER_MICROSECOND / mInnerLoops;
        }

        /**
         * Calculate the stats for the data. Note the data in the range will be
         * sorted.
         *
         * @param data
         * @param count
         */
        public Stats calculate(long data[], int count) {
            if (count == 1) {
                mMin = mMax = data[0];
                mTotal = mMedian = mMean = data[0];
                mStdDev = 0;
            } else if (count > 1) {
                Arrays.sort(data, 0, count);
                mMin = data[0];
                mMax = data[count - 1];
                if ((count & 1) == 1) {
                    mMedian = data[((count + 1) / 2) - 1];
                } else {
                    mMedian = (data[count / 2] + data[(count / 2) - 1]) / 2;
                }
                mTotal = 0;
                double sumSquares = 0;
                for (int i = 0; i < count; i++) {
                    long t = data[i];
                    mTotal += t;
                    sumSquares += t * t;
                }
                mMean = mTotal / count;
                double variance = (sumSquares / count) - (mMean * mMean);
                mStdDev = Math.pow(variance, 0.5);
            } else {
                init();
            }
            return this;
        }

        /** Convert to string */
        @Override
            public String toString() {
                double timePerLoop = getTimePerLoop();
                double stdDevPerLoop = mStdDev / TICKS_PER_MICROSECOND / mInnerLoops;
                return String.format(
                        "tpl=%,.6fus stdDev=%,.6fus tpl/stdDev=%.2fpercent min=%,.6fus median=%,.6fus mean=%,.6fus max=%,.6fus total=%,.6fs il=%d, ol=%d tlc=%d",
                        timePerLoop, stdDevPerLoop, (stdDevPerLoop / timePerLoop) * 100, mMin
                        / TICKS_PER_MICROSECOND, mMedian / TICKS_PER_MICROSECOND, mMean
                        / TICKS_PER_MICROSECOND, mMax / TICKS_PER_MICROSECOND, mTotal
                        / (TICKS_PER_MICROSECOND * 1000000.0), mInnerLoops, mOuterLoops, mStdDevTooLargeCount);
            }
    }

    /** Statistics */
    private Stats mStats = new Stats();

    /** Statistics of the clock precision */
    private Stats mClockStats;

    /** Number of items in times array */
    private int mCount;

    /** Array of stop - start times */
    private long mTimes[];

    /** Time of last started */
    private long mStart;

    /** Sleep a little so we don't look like a hog */
    private void sleep() {
        try {
            Thread.sleep(0);
        } catch (InterruptedException e) {
            // Ignore exception
        }
    }

    /** Empty Runnable used for determining overhead */
    private Runnable mEmptyRunnable = new Runnable() {
        public void run() {
        }
    };

    /** Initialize */
    private void init(int maxCount, CalibrationRec cr) {
        mTimes = new long[maxCount];
        mCr = cr;
        reset();
    }

    /** Construct the stop watch */
    public PerfTimer() {
        init(10, new CalibrationRec());
    }

    /** Construct setting size of times array */
    public PerfTimer(int maxCount) {
        init(maxCount, new CalibrationRec());
    }

    /** Construct the stop watch */
    public PerfTimer(CalibrationRec cr) {
        init(10, cr);
    }

    /** Construct the stop watch */
    public PerfTimer(int maxCount, CalibrationRec cr) {
        init(maxCount, cr);
    }

    /** Reset the contents of the times array */
    public PerfTimer reset() {
        mCount = 0;
        mStats.init();
        return this;
    }

    /** Reset and then start the timer */
    public PerfTimer restart() {
        reset();
        mStart = getTicks();
        return this;
    }

    /** Start timing */
    public PerfTimer start() {
        mStart = getTicks();
        return this;
    }

    /**
     * Record the difference between start and now in the times array
     * incrementing count. The time will be stored in the times array if the
     * array is not full.
     */
    public PerfTimer stop() {
        long stop = getTicks();
        if (mCount < mTimes.length) {
            mTimes[mCount++] = stop - mStart;
        }
        return this;
    }

    /**
     * Time how long it takes to execute runnable.run() innerLoop number of
     * times outerLoops number of times.
     *
     * @param outerLoops
     * @param innerLoops
     * @param runnable
     * @return PerfTimer
     */
    public PerfTimer timeEach(Stats stats, int outerLoops, int innerLoops, Runnable runnable) {
        reset();
        resize(outerLoops);
        stats.setOuterLoops(outerLoops);
        stats.setInnerLoops(innerLoops);
        for (int i = 0; i < outerLoops; i++) {
            start();
            for (int j = 0; j < innerLoops; j++) {
                runnable.run();
            }
            stop();
            sleep();
        }
        return this;
    }

    /**
     * Time how long it takes to execute runnable.run(). Runs runnable votes
     * times and returns the Stats of the fastest run. The actual number times
     * that runnable.run() is executes is enough times so that it runs at least
     * minThreadholeInMicros but not greater than maxThreadholdInMicro. This
     * minimizes the chance that long context switches influence the result.
     *
     * @param votes is the number of runnable will be executed to determine
     *            fastest run
     * @param outerLoops is the number of of times the inner loop is run
     * @param initialInnerLoops is the initial inner loop
     * @param maxStdDevRetrys if the maxStdDevRatio is exceeded this number of
     *            time the PerfTimerException is thrown.
     * @param maxStdDevRatio the ratio of the standard deviation of the run and
     *            the time to run.
     * @param debugLevel DEBUG_LEVEL_NONE, DEBUG_LEVEL_SOME, DEBUG_LEVEL_ALL
     * @param runnable is the code to test.
     * @return Stats of the fastest run.
     */
    public Stats timeEachAutomatically(int votes, int outerLoops, int initialInnerLoops,
            double minThresholdInMicros, double maxThresholdInMicros, int maxStdDevRetrys,
            double maxStdDevRatio, int debugLevel, Runnable runnable) throws PerfTimerException {
        Stats minStats = null;

        for (int v = 0; v < votes; v++) {
            boolean successful = false;
            Stats stats = new Stats();
            int innerLoops = initialInnerLoops;

            /* Warm up cache */
            timeEach(stats, outerLoops, initialInnerLoops, runnable);

            for (int stdDevRetrys = 0; stdDevRetrys < maxStdDevRetrys; stdDevRetrys++) {
                /**
                 * First time may be long enough
                 */
                timeEach(stats, outerLoops, innerLoops, runnable);
                analyze(stats, mTimes, outerLoops, debugLevel);
                double innerLoopTime = stats.getMinInMicros();
                if ((innerLoopTime >= minThresholdInMicros
                        - ((maxThresholdInMicros - minThresholdInMicros) / 2))) {
                    if (stats.stdDevOk(maxStdDevRatio)) {
                        successful = true;
                        break;
                    } else {
                        stats.incStdDevTooLargeCount();
                        if (debugLevel >= DEBUG_LEVEL_SOME) {
                            log(String.format(
                                    "tea: tlc=%d StdDevRatio=%.2f > maxStdDevRatio=%.2f",
                                    stats.getStdDevTooLargeCount(), stats.getStdDevRatio(),
                                    maxStdDevRatio));
                        }
                    }
                } else {
                    /**
                     * The initial number of loops is too short find the number
                     * of loops that exceeds maxThresholdInMicros. Then use a
                     * binary search to find the approriate innerLoop value that
                     * is between min/maxThreshold.
                     */
                    innerLoops *= 10;
                    int maxInnerLoops = innerLoops;
                    int minInnerLoops = 1;
                    boolean binarySearch = false;
                    for (int i = 0; i < 10; i++) {
                        timeEach(stats, outerLoops, innerLoops, runnable);
                        analyze(stats, mTimes, outerLoops, debugLevel);
                        innerLoopTime = stats.getMedianInMicros();
                        if ((innerLoopTime >= minThresholdInMicros)
                                && (innerLoopTime <= maxThresholdInMicros)) {
                            if (stats.stdDevOk(maxStdDevRatio)) {
                                successful = true;
                                break;
                            } else {
                                stats.incStdDevTooLargeCount();
                                if (debugLevel >= DEBUG_LEVEL_SOME) {
                                    log(String.format(
                                         "tea: tlc=%d StdDevRatio=%.2f > maxStdDevRatio=%.2f",
                                         stats.getStdDevTooLargeCount(), stats.getStdDevRatio(),
                                         maxStdDevRatio));
                                }
                            }
                        } else if (binarySearch) {
                            if ((innerLoopTime < minThresholdInMicros)) {
                                minInnerLoops = innerLoops;
                            } else {
                                maxInnerLoops = innerLoops;
                            }
                            innerLoops = (maxInnerLoops + minInnerLoops) / 2;
                        } else if (innerLoopTime >= maxThresholdInMicros) {
                            /* Found a too large value, change to binary search */
                            binarySearch = true;
                            maxInnerLoops = innerLoops;
                            innerLoops = (maxInnerLoops + minInnerLoops) / 2;
                        } else {
                            innerLoops *= 10;
                        }
                    }
                    if (successful) {
                        break;
                    }
                }
            }
            if (!successful) {
                /* Couldn't find the number of loops to execute */
                throw new PerfTimerException();
            }

            /** Looking for minimum */
            if ((minStats == null) || (minStats.getTimePerLoop() > stats.getTimePerLoop())) {
                minStats = stats;
            }
            if (debugLevel >= DEBUG_LEVEL_SOME) {
                log(String.format("minStats.getTimePerLoop=%f minStats: %s", minStats.getTimePerLoop(), minStats));
            }
        }

        return minStats;
    }

    /**
     * Time how long it takes to execute runnable.run() with a threshold of 1 to
     * 10ms.
     *
     * @param debugLevel DEBUG_LEVEL_NONE, DEBUG_LEVEL_SOME, DEBUG_LEVEL_ALL
     * @param runnable
     * @throws PerfTimerException
     */
    public Stats timeEachAutomatically(int debugLevel, Runnable runnable)
            throws PerfTimerException {
        mStats = timeEachAutomatically(mCr.mVotes, OUTER_LOOPS, 1, mCr.mMinThresholdInMicros,
                mCr.mMaxThresholdInMicros, mCr.mStdDevRetrys, mCr.mMaxStdDevRatio, debugLevel,
                runnable);
        return mStats;
    }

    /**
     * Time how long it takes to execute runnable.run() with a threshold of 1 to
     * 10ms.
     *
     * @param runnable
     * @throws PerfTimerException
     */
    public Stats timeEachAutomatically(Runnable runnable) throws PerfTimerException {
        mStats = timeEachAutomatically(mCr.mVotes, OUTER_LOOPS, 1, mCr.mMinThresholdInMicros,
                mCr.mMaxThresholdInMicros, mCr.mStdDevRetrys, mCr.mMaxStdDevRatio,
                DEBUG_LEVEL_NONE, runnable);
        return mStats;
    }

    /** Resize the times array */
    public void resize(int maxCount) {
        if (maxCount > mTimes.length) {
            mTimes = new long[maxCount];
        }
    }

    /**
     * Analyze the data calculating the min, max, total, median, mean and
     * stdDev. The standard deviation is calculated as sqrt(((sum of the squares
     * of each time) / count) - mean^2)
     * {@link "http://www.sciencebuddies.org/mentoring/project_data_analysis_variance_std_deviation.shtml"}
     *
     * @param debugLevel DEBUG_LEVEL_NONE, DEBUG_LEVEL_SOME, DEBUG_LEVEL_ALL
     * @return StopWatch
     */
    public Stats analyze(Stats stats, long data[], int count, int debugLevel) {
        if (count > 0) {
            if (debugLevel >= DEBUG_LEVEL_ALL) {
                for (int j = 0; j < count; j++) {
                    log(String.format("data[%d]=%,dns", j, data[j]));
                }
            }
            stats.calculate(data, count);
        } else {
            stats.init();
        }
        if (debugLevel >= DEBUG_LEVEL_SOME) {
            log("stats: " + stats);
        }
        return stats;
    }

    /**
     * Calibrate the system and set it for this PerfTimer instance
     *
     * @param debugLevel DEBUG_LEVEL_NONE, DEBUG_LEVEL_SOME, DEBUG_LEVEL_ALL
     * @param precisionInDecimalDigits the precision in number of decimal digits
     */
    public CalibrationRec calibrate(int debugLevel, double precisionInDecimalDigits)
            throws PerfTimerException {
        int nonZeroCount = 0;
        Stats stats = new Stats();
        CalibrationRec cr = new CalibrationRec();

        /* initialize the precision */
        cr.mPrecisionInDecimalDigits = precisionInDecimalDigits;

        /* Warm up the cache */
        timeEach(stats, OUTER_LOOPS, 10, mEmptyRunnable);

        /*
         * Determine the clock stats with at least 20% non-zero unique values.
         */
        for (int clockStatsTries = 1; clockStatsTries < 100; clockStatsTries++) {
            int j;
            int i;
            long cur;
            long prev;
            long min;

            int innerLoops = clockStatsTries * 10;
            timeEach(stats, OUTER_LOOPS, innerLoops, mEmptyRunnable);
            long nonZeroValues[] = new long[mCount];
            prev = 0;
            for (nonZeroCount = 0, i = 0; i < mCount; i++) {
                cur = mTimes[i];
                if (cur > 0) {
                    nonZeroValues[nonZeroCount++] = cur;
                }
            }
            if (nonZeroCount > (mCount * 0.20)) {
                // Calculate thresholds
                analyze(stats, nonZeroValues, nonZeroCount, debugLevel);
                stats.calculate(nonZeroValues, nonZeroCount);
                cr.mMinThresholdInMicros = stats.getMeanInMicros()
                        * Math.pow(10, cr.mPrecisionInDecimalDigits);
                cr.mMaxThresholdInMicros = cr.mMinThresholdInMicros * 2;

                // Set overhead to 0 and time the empty loop then set overhead.
                cr.mRunnableOverheadInMicros = 0;
                mClockStats = timeEachAutomatically(mCr.mVotes, OUTER_LOOPS, innerLoops,
                        cr.mMinThresholdInMicros, cr.mMaxThresholdInMicros, mCr.mStdDevRetrys,
                        mCr.mMaxStdDevRatio, debugLevel, mEmptyRunnable);
                cr.mRunnableOverheadInMicros = mClockStats.getMinInMicros()
                        / mClockStats.getInnerLoops();
                break;
            }
            nonZeroCount = 0;
        }
        if (nonZeroCount == 0) {
            throw new PerfTimerException();
        }
        if (debugLevel >= DEBUG_LEVEL_SOME) {
            log(String.format("calibrate X oh=%.6fus minT=%,.6fus maxT=%,.6fus stats: %s",
                    cr.mRunnableOverheadInMicros, cr.mMinThresholdInMicros,
                    cr.mMaxThresholdInMicros, stats));
        }
        mCr = cr;
        return mCr;
    }

    /** Calibrate the system and set it for this PerfTimer instance */
    public CalibrationRec calibrate(double precisionInDecimalDigits) throws PerfTimerException {
        return calibrate(DEBUG_LEVEL_NONE, precisionInDecimalDigits);
    }

    /** Calibrate the system and set it for this PerfTimer instance */
    public CalibrationRec calibrate() throws PerfTimerException {
        return calibrate(DEBUG_LEVEL_NONE, mCr.mPrecisionInDecimalDigits);
    }

    /*
     * Accessors for the private data
     */

    /** Set calibration record */
    public void setCalibrationRec(CalibrationRec cr) {
        mCr = cr;
    }

    /** Get calibration record */
    public CalibrationRec getCalibrationRec() {
        return mCr;
    }

    /** Number of samples in times array. */
    public int getCount() {
        return mCount;
    }

    /** Minimum value of collected data in microseconds, valid after analyze. */
    public double getMinInMicros() {
        return mStats.getMinInMicros();
    }

    /** Maximum value of collected data in microseconds, valid after analyze. */
    public double getMaxInMicros() {
        return mStats.getMaxInMicros();
    }

    /**
     * Sum of the values of collected data in microseconds, valid after analyze.
     */
    public double getTotalInMicros() {
        return mStats.getTotalInMicros();
    }

    /** Sum of the values of collected data in seconds, valid after analyze. */
    public double getTotalInSecs() {
        return mStats.getTotalInSecs();
    }

    /** Sum of the values of collected data in seconds, valid after analyze. */
    public double getMeanInMicros() {
        return mStats.getMeanInMicros();
    }

    /** Median value of collected data in microseconds, valid after analyze. */
    public double getMedianInMicros() {
        return mStats.getMedianInMicros();
    }

    /**
     * Standard deviation of collected data in microseconds, valid after
     * analyze.
     */
    public double getStdDevInMicros() {
        return mStats.getStdDevInMicros();
    }

    /** The mTimes[index] value */
    public long getTime(int index) {
        return mTimes[index];
    }

    /** The mTimes */
    public long[] getTimes() {
        return mTimes;
    }

    /** @return the clock stats as measured in calibrate */
    public Stats getClockStats() {
        return mClockStats;
    }

    /** @return the stats */
    public Stats getStats() {
        return mStats;
    }

    /**
     * Convert stats to string
     *
     * @param debugLevel DEBUG_LEVEL_NONE, DEBUG_LEVEL_SOME, DEBUG_LEVEL_ALL
     */
    public String stats(int debugLevel) {
        int innerLoops = mStats.getInnerLoops();
        if (mCount == 0) {
            return String.format("%,.3fus", (getTicks() - mStart) / TICKS_PER_MICROSECOND);
        } else {
            if (mCount == 1) {
                return String.format("%,.3fus", getTime());
            } else {
                analyze(mStats, mTimes, mCount, debugLevel);
                return mStats.toString();
            }
        }
    }

    /**
     * Convert string
     */
    public String stats() {
        return stats(0);
    }

    /**
     * Get time
     */
    public double getTime() {
        int innerLoops = mStats.getInnerLoops();
        if (mCount == 0) {
            return (getTicks() - mStart) / TICKS_PER_MICROSECOND;
        } else {
            if (mCount == 1) {
                return mStats.getTotalInMicros();
            } else {
                analyze(mStats, mTimes, mCount, DEBUG_LEVEL_NONE);
                return (mStats.getMinInMicros() / innerLoops) - mCr.mRunnableOverheadInMicros;
            }
        }
    }

    /** Convert to string */
    @Override
    public String toString() {
        return String.format("%,.3fus", getTime());
    }
}
