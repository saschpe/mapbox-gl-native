package com.mapbox.mapboxsdk.offline;

/**
 * A region's status includes its active/inactive state as well as counts
 * of the number of resources that have completed downloading, their total
 * size in bytes, and the total number of resources that are required.
 *
 * Note that the total required size in bytes is not currently available. A
 * future API release may provide an estimate of this number.
 */
public class OfflineRegionStatus {

    @OfflineRegion.DownloadState private int downloadState = OfflineRegion.STATE_INACTIVE;

    /**
     * The number of resources that have been fully downloaded and are ready for
     * offline access.
     */
    private long completedResourceCount = 0;

    /**
     * The cumulative size, in bytes, of all resources that have been fully downloaded.
     */
    private long completedResourceSize = 0;

    /**
     * The number of resources that are known to be required for this region. See the
     * documentation for `requiredResourceCountIsIndeterminate` for an important caveat
     * about this number.
     */
    private long requiredResourceCount = 0;

    /**
     * This property is true during early phases of an offline download, when the total
     * required resource count is unknown and requiredResourceCount is merely a lower
     * bound.
     *
     * Specifically, it is true before until the style and tile sources have been
     * downloaded, and false thereafter.
     */
    private boolean requiredResourceCountIsIndeterminate = true;

    /*
     * Use setObserver(OfflineRegionObserver observer) to obtain a OfflineRegionStatus object.
     */

    private OfflineRegionStatus() {
        // For JNI use only
    }

    /*
     * Is the region complete?
     */

    public boolean complete() {
        return (completedResourceCount == requiredResourceCount);
    }

    /*
     * Getters
     */

    public @OfflineRegion.DownloadState int getDownloadState() {
        return downloadState;
    }

    public long getCompletedResourceCount() {
        return completedResourceCount;
    }

    public long getCompletedResourceSize() {
        return completedResourceSize;
    }

    public long getRequiredResourceCount() {
        return requiredResourceCount;
    }

    public boolean isRequiredResourceCountPrecise() {
        return requiredResourceCountIsIndeterminate;
    }

}
