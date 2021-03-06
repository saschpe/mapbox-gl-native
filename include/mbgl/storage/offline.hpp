#pragma once

#include <mbgl/util/geo.hpp>
#include <mbgl/util/optional.hpp>
#include <mbgl/style/types.hpp>
#include <mbgl/storage/response.hpp>

#include <string>
#include <vector>
#include <functional>

namespace mbgl {

class TileID;
class SourceInfo;

/*
 * An offline region defined by a style URL, geographic bounding box, zoom range, and
 * device pixel ratio.
 *
 * Both minZoom and maxZoom must be ≥ 0, and maxZoom must be ≥ minZoom.
 *
 * maxZoom may be ∞, in which case for each tile source, the region will include
 * tiles from minZoom up to the maximum zoom level provided by that source.
 *
 * pixelRatio must be ≥ 0 and should typically be 1.0 or 2.0.
 */
class OfflineTilePyramidRegionDefinition {
public:
    OfflineTilePyramidRegionDefinition(const std::string&, const LatLngBounds&, double, double, float);

    /* Private */
    std::vector<TileID> tileCover(SourceType, uint16_t tileSize, const SourceInfo&) const;

    const std::string styleURL;
    const LatLngBounds bounds;
    const double minZoom;
    const double maxZoom;
    const float pixelRatio;
};

/*
 * For the present, a tile pyramid is the only type of offline region. In the future,
 * other definition types will be available and this will be a variant type.
 */
using OfflineRegionDefinition = OfflineTilePyramidRegionDefinition;

/*
 * The encoded format is private.
 */
std::string encodeOfflineRegionDefinition(const OfflineRegionDefinition&);
OfflineRegionDefinition decodeOfflineRegionDefinition(const std::string&);

/*
 * Arbitrary binary region metadata. The contents are opaque to the mbgl implementation;
 * it just stores and retrieves a BLOB. SDK bindings should leave the interpretation of
 * this data up to the application; they _should not_ enforce a higher-level data format.
 * In the future we want offline database to be portable across target platforms, and a
 * platform-specific metadata format would prevent that.
 */
using OfflineRegionMetadata = std::vector<uint8_t>;

/*
 * A region is either inactive (not downloading, but previously-downloaded
 * resources are available for use), or active (resources are being downloaded
 * or will be downloaded, if necessary, when network access is available).
 *
 * This state is independent of whether or not the complete set of resources
 * is currently available for offline use. To check if that is the case, use
 * `OfflineRegionStatus::complete()`.
 */
enum class OfflineRegionDownloadState {
    Inactive,
    Active
};

/*
 * A region's status includes its active/inactive state as well as counts
 * of the number of resources that have completed downloading, their total
 * size in bytes, and the total number of resources that are required.
 *
 * Note that the total required size in bytes is not currently available. A
 * future API release may provide an estimate of this number.
 */
class OfflineRegionStatus {
public:
    OfflineRegionDownloadState downloadState = OfflineRegionDownloadState::Inactive;

    /**
     * The number of resources that have been fully downloaded and are ready for
     * offline access.
     */
    uint64_t completedResourceCount = 0;

    /**
     * The cumulative size, in bytes, of all resources that have been fully downloaded.
     */
    uint64_t completedResourceSize = 0;

    /**
     * The number of resources that are known to be required for this region. See the
     * documentation for `requiredResourceCountIsIndeterminate` for an important caveat
     * about this number.
     */
    uint64_t requiredResourceCount = 0;

    /**
     * This property is true during early phases of an offline download, when the total
     * required resource count is unknown and requiredResourceCount is merely a lower
     * bound.
     *
     * Specifically, it is true before until the style and tile sources have been
     * downloaded, and false thereafter.
     */
    bool requiredResourceCountIsIndeterminate = true;

    bool complete() const {
        return completedResourceCount == requiredResourceCount;
    }
};

/*
 * A region can have a single observer, which gets notified whenever a change
 * to the region's status occurs.
 */
class OfflineRegionObserver {
public:
    virtual ~OfflineRegionObserver() = default;

    /*
     * Implement this method to be notified of a change in the status of an
     * offline region. Status changes include any change in state of the members
     * of OfflineRegionStatus.
     *
     * Note that this method will be executed on the database thread; it is the
     * responsibility of the SDK bindings to wrap this object in an interface that
     * re-executes the user-provided implementation on the main thread.
     */
    virtual void statusChanged(OfflineRegionStatus) {}

    /*
     * Implement this method to be notified of errors encountered while downloading
     * regional resources. Such errors may be recoverable; for example the implementation
     * will attempt to re-request failed resources based on an exponential backoff
     * algorithm, or when it detects that network access has been restored.
     *
     * Note that this method will be executed on the database thread; it is the
     * responsibility of the SDK bindings to wrap this object in an interface that
     * re-executes the user-provided implementation on the main thread.
     */
    virtual void responseError(Response::Error) {}
};

class OfflineRegion {
public:
    // Move-only; not publicly constructible.
    OfflineRegion(OfflineRegion&&);
    OfflineRegion& operator=(OfflineRegion&&);
    ~OfflineRegion();

    OfflineRegion() = delete;
    OfflineRegion(const OfflineRegion&) = delete;
    OfflineRegion& operator=(const OfflineRegion&) = delete;

    int64_t getID() const;
    const OfflineRegionDefinition& getDefinition() const;
    const OfflineRegionMetadata& getMetadata() const;

private:
    friend class OfflineDatabase;

    OfflineRegion(int64_t id,
                  const OfflineRegionDefinition&,
                  const OfflineRegionMetadata&);

    const int64_t id;
    const OfflineRegionDefinition definition;
    const OfflineRegionMetadata metadata;
};

} // namespace mbgl
