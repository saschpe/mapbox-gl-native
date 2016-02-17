package com.mapbox.mapboxsdk.offline;

import android.os.Handler;
import android.os.Looper;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * For the present, a tile pyramid is the only type of offline region. In the future,
 * other definition types will be available and this will be a variant type.
 */
public class OfflineRegion {

    // Parent OfflineManager
    private OfflineManager offlineManager;

    // Members
    private long mId = 0;
    private OfflineRegionDefinition mDefinition = null;
    private OfflineRegionMetadata mMetadata = null;

    // Holds the pointer to JNI OfflineRegion
    private long mOfflineRegionPtr = 0;

    // Makes sure callbacks come back to the main thread
    private Handler handler;

    /**
     * A region can have a single observer, which gets notified whenever a change
     * to the region's status occurs.
     */
    public interface OfflineRegionObserver {
        /**
         * Implement this method to be notified of a change in the status of an
         * offline region. Status changes include any change in state of the members
         * of OfflineRegionStatus.
         *
         * Note that this method will be executed on the database thread; it is the
         * responsibility of the SDK bindings to wrap this object in an interface that
         * re-executes the user-provided implementation on the main thread.
         */
        void onStatusChanged(OfflineRegionStatus status);

        /**
         * Implement this method to be notified of errors encountered while downloading
         * regional resources. Such errors may be recoverable; for example the implementation
         * will attempt to re-request failed resources based on an exponential backoff
         * algorithm, or when it detects that network access has been restored.
         *
         * Note that this method will be executed on the database thread; it is the
         * responsibility of the SDK bindings to wrap this object in an interface that
         * re-executes the user-provided implementation on the main thread.
         */
        void onError(OfflineRegionError error);
    }

    /*
     * Callbacks
     */

    public interface OfflineRegionStatusCallback {
        void onStatus(OfflineRegionStatus status);
        void onError(String error);
    }

    public interface OfflineRegionDeleteCallback {
        void onDelete();
        void onError(String error);
    }

    /**
     * A region is either inactive (not downloading, but previously-downloaded
     * resources are available for use), or active (resources are being downloaded
     * or will be downloaded, if necessary, when network access is available).
     *
     * This state is independent of whether or not the complete set of resources
     * is currently available for offline use. To check if that is the case, use
     * `OfflineRegionStatus::complete()`.
     */

    @IntDef({STATE_INACTIVE, STATE_ACTIVE})
    @Retention(RetentionPolicy.SOURCE)
    public @interface DownloadState {}

    public static final int STATE_INACTIVE = 0;
    public static final int STATE_ACTIVE = 1;

    /*
     * Constructor
     */

    private OfflineRegion() {
        // For JNI use only, to create a new offline region, use
        // OfflineManager.createOfflineRegion() instead.
    }

    /*
     * Getters
     */

    public long getID() {
        return mId;
    }

    public OfflineRegionDefinition getDefinition() {
        return mDefinition;
    }

    public OfflineRegionMetadata getMetadata() {
        return mMetadata;
    }

    private Handler getHandler() {
        if (handler == null) {
            handler = new Handler(Looper.getMainLooper());
        }

        return handler;
    }

    /**
     * Register an observer to be notified when the state of the region changes.
     */
    public void setObserver(@NonNull final OfflineRegionObserver observer) {
        setOfflineRegionObserver(this, new OfflineRegionObserver() {
            @Override
            public void onStatusChanged(final OfflineRegionStatus status) {
                getHandler().post(new Runnable() {
                    @Override
                    public void run() {
                        observer.onStatusChanged(status);
                    }
                });
            }

            @Override
            public void onError(final OfflineRegionError error) {
                getHandler().post(new Runnable() {
                    @Override
                    public void run() {
                        observer.onError(error);
                    }
                });
            }
        });
    }

    /**
     * Pause or resume downloading of regional resources.
     */
    public void setDownloadState(@DownloadState int state) {
        setOfflineRegionDownloadState(this, state);
    }

    /**
     * Retrieve the current status of the region. The query will be executed
     * asynchronously and the results passed to the given callback, which will be
     * executed on the database thread; it is the responsibility of the SDK bindings
     * to re-execute a user-provided callback on the main thread.
     */
    public void getStatus(@NonNull final OfflineRegionStatusCallback callback) {
        getOfflineRegionStatus(this, new OfflineRegionStatusCallback() {
            @Override
            public void onStatus(final OfflineRegionStatus status) {
                getHandler().post(new Runnable() {
                    @Override
                    public void run() {
                        callback.onStatus(status);
                    }
                });
            }

            @Override
            public void onError(final String error) {
                getHandler().post(new Runnable() {
                    @Override
                    public void run() {
                        callback.onError(error);
                    }
                });
            }
        });
    }

    /**
     * Remove an offline region from the database and perform any resources evictions
     * necessary as a result.
     *
     * Eviction works by removing the least-recently requested resources not also required
     * by other regions, until the database shrinks below a certain size.
     *
     * Note that this method takes ownership of the input, reflecting the fact that once
     * region deletion is initiated, it is not legal to perform further actions with the
     * region.
     *
     * When the operation is complete or encounters an error, the given callback will be
     * executed on the database thread; it is the responsibility of the SDK bindings
     * to re-execute a user-provided callback on the main thread.
     */
    public void delete(@NonNull final OfflineRegionDeleteCallback callback) {
        deleteOfflineRegion(this, new OfflineRegionDeleteCallback() {
            @Override
            public void onDelete() {
                getHandler().post(new Runnable() {
                    @Override
                    public void run() {
                        callback.onDelete();
                    }
                });
            }

            @Override
            public void onError(final String error) {
                getHandler().post(new Runnable() {
                    @Override
                    public void run() {
                        callback.onError(error);
                    }
                });
            }
        });
    }

    /*
     * Native methods
     */

    private native void setOfflineRegionObserver(
            OfflineRegion offlineRegion,
            OfflineRegionObserver observerCallback);

    private native void setOfflineRegionDownloadState(
            OfflineRegion offlineRegion,
            @DownloadState int offlineRegionDownloadState);

    private native void getOfflineRegionStatus(
            OfflineRegion offlineRegion,
            OfflineRegionStatusCallback statusCallback);

    private native void deleteOfflineRegion(
            OfflineRegion offlineRegion,
            OfflineRegionDeleteCallback deleteCallback);

}
