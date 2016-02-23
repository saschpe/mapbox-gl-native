package com.mapbox.mapboxsdk.offline;

/**
 * An Offline Region error
 */
public class OfflineRegionError {

    /**
     * Success = 1
     * NotFound = 2
     * Server = 3
     * Connection = 4
     * Other = 6
     */
    private int reason;

    /**
    /* An error message from the request handler, e.g. a server message or a system message
    /* informing the user about the reason for the failure.
     */
    private String message;

    /*
     * Constructors
     */

    private OfflineRegionError() {
        // For JNI use only
    }

    public OfflineRegionError(int reason, String message) {
        this.reason = reason;
        this.message = message;
    }

    /*
     * Getters
     */

    public int getReason() {
        return reason;
    }

    public String getMessage() {
        return message;
    }
}
