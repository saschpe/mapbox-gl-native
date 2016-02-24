#include <mbgl/storage/offline_database.hpp>
#include <mbgl/storage/response.hpp>
#include <mbgl/util/compression.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/util/string.hpp>
#include <mbgl/util/chrono.hpp>
#include <mbgl/map/tile_id.hpp>
#include <mbgl/platform/log.hpp>

#include "sqlite3.hpp"
#include <sqlite3.h>

namespace mbgl {

using namespace mapbox::sqlite;

// If you change the schema you must write a migration from the previous version.
static const uint32_t schemaVersion = 2;

OfflineDatabase::Statement::~Statement() {
    stmt.reset();
    stmt.clearBindings();
}

OfflineDatabase::OfflineDatabase(const std::string& path_, uint64_t maximumCacheSize_)
    : path(path_),
      maximumCacheSize(maximumCacheSize_) {
    ensureSchema();
}

OfflineDatabase::~OfflineDatabase() {
    // Deleting these SQLite objects may result in exceptions, but we're in a destructor, so we
    // can't throw anything.
    try {
        statements.clear();
        db.reset();
    } catch (mapbox::sqlite::Exception& ex) {
        Log::Error(Event::Database, ex.code, ex.what());
    }
}

void OfflineDatabase::ensureSchema() {
    if (path != ":memory:") {
        try {
            db = std::make_unique<Database>(path.c_str(), ReadWrite);
            db->setBusyTimeout(Milliseconds::max());

            {
                auto userVersionStmt = db->prepare("PRAGMA user_version");
                userVersionStmt.run();
                switch (userVersionStmt.get<int>(0)) {
                case 0: break; // cache-only database; ok to delete
                case 1: break; // cache-only database; ok to delete
                case 2: return;
                default: throw std::runtime_error("unknown schema version");
                }
            }

            removeExisting();
            db = std::make_unique<Database>(path.c_str(), ReadWrite | Create);
            db->setBusyTimeout(Milliseconds::max());
        } catch (mapbox::sqlite::Exception& ex) {
            if (ex.code == SQLITE_CANTOPEN) {
                db = std::make_unique<Database>(path.c_str(), ReadWrite | Create);
                db->setBusyTimeout(Milliseconds::max());
            } else if (ex.code == SQLITE_NOTADB) {
                removeExisting();
                db = std::make_unique<Database>(path.c_str(), ReadWrite | Create);
                db->setBusyTimeout(Milliseconds::max());
            }
        }
    }

    #include "offline_schema.cpp.include"

    db = std::make_unique<Database>(path.c_str(), ReadWrite | Create);
    db->setBusyTimeout(Milliseconds::max());
    db->exec(schema);
    db->exec("PRAGMA user_version = " + util::toString(schemaVersion));
}

void OfflineDatabase::removeExisting() {
    Log::Warning(Event::Database, "Removing existing incompatible offline database");

    db.reset();

    try {
        util::deleteFile(path);
    } catch (util::IOException& ex) {
        Log::Error(Event::Database, ex.code, ex.what());
    }
}

OfflineDatabase::Statement OfflineDatabase::getStatement(const char * sql) {
    auto it = statements.find(sql);

    if (it != statements.end()) {
        return Statement(*it->second);
    }

    return Statement(*statements.emplace(sql, std::make_unique<mapbox::sqlite::Statement>(db->prepare(sql))).first->second);
}

optional<Response> OfflineDatabase::get(const Resource& resource) {
    if (resource.kind == Resource::Kind::Tile) {
        assert(resource.tileData);
        return getTile(*resource.tileData);
    } else {
        return getResource(resource);
    }
}

std::pair<bool, uint64_t> OfflineDatabase::put(const Resource& resource, const Response& response) {
    return putInternal(resource, response, true);
}

std::pair<bool, uint64_t> OfflineDatabase::putInternal(const Resource& resource, const Response& response, bool evict_) {
    if (response.error) {
        return { false, 0 };
    }

    std::string compressedData;
    bool compressed = false;
    uint64_t size = 0;

    if (response.data) {
        compressedData = util::compress(*response.data);
        compressed = compressedData.size() < response.data->size();
        size = compressed ? compressedData.size() : response.data->size();
    }

    if (evict_ && !evict(size)) {
        Log::Warning(Event::Database, "Unable to make space for entry");
        return { false, 0 };
    }

    bool inserted;

    if (resource.kind == Resource::Kind::Tile) {
        assert(resource.tileData);
        inserted = putTile(*resource.tileData, response,
                compressed ? compressedData : *response.data,
                compressed);
    } else {
        inserted = putResource(resource, response,
                compressed ? compressedData : *response.data,
                compressed);
    }

    return { inserted, size };
}

optional<Response> OfflineDatabase::getResource(const Resource& resource) {
    Statement accessedStmt = getStatement(
        "UPDATE resources SET accessed = ?1 WHERE url = ?2");

    accessedStmt->bind(1, SystemClock::now());
    accessedStmt->bind(2, resource.url);
    accessedStmt->run();

    Statement stmt = getStatement(
        //        0      1        2       3        4
        "SELECT etag, expires, modified, data, compressed "
        "FROM resources "
        "WHERE url = ?");

    stmt->bind(1, resource.url);

    if (!stmt->run()) {
        return {};
    }

    Response response;

    response.etag     = stmt->get<optional<std::string>>(0);
    response.expires  = stmt->get<optional<SystemTimePoint>>(1);
    response.modified = stmt->get<optional<SystemTimePoint>>(2);

    optional<std::string> data = stmt->get<optional<std::string>>(3);
    if (!data) {
        response.noContent = true;
    } else if (stmt->get<int>(4)) {
        response.data = std::make_shared<std::string>(util::decompress(*data));
    } else {
        response.data = std::make_shared<std::string>(*data);
    }

    return response;
}

bool OfflineDatabase::putResource(const Resource& resource,
                                  const Response& response,
                                  const std::string& data,
                                  bool compressed) {
    if (response.notModified) {
        Statement update = getStatement(
            "UPDATE resources "
            "SET accessed = ?1, "
            "    expires  = ?2 "
            "WHERE url    = ?3 ");

        update->bind(1, SystemClock::now());
        update->bind(2, response.expires);
        update->bind(3, resource.url);
        update->run();
        return false;
    }

    // We can't use REPLACE because it would change the id value.

    Statement update = getStatement(
        "UPDATE resources "
        "SET kind       = ?1, "
        "    etag       = ?2, "
        "    expires    = ?3, "
        "    modified   = ?4, "
        "    accessed   = ?5, "
        "    data       = ?6, "
        "    compressed = ?7 "
        "WHERE url      = ?8 ");

    update->bind(1, int(resource.kind));
    update->bind(2, response.etag);
    update->bind(3, response.expires);
    update->bind(4, response.modified);
    update->bind(5, SystemClock::now());
    update->bind(8, resource.url);

    if (response.noContent) {
        update->bind(6, nullptr);
        update->bind(7, false);
    } else {
        update->bindBlob(6, data.data(), data.size(), false);
        update->bind(7, compressed);
    }

    update->run();
    if (db->changes() != 0) {
        return false;
    }

    Statement insert = getStatement(
        "INSERT INTO resources (url, kind, etag, expires, modified, accessed, data, compressed) "
        "VALUES                (?1,  ?2,   ?3,   ?4,      ?5,       ?6,       ?7,   ?8) ");

    insert->bind(1, resource.url);
    insert->bind(2, int(resource.kind));
    insert->bind(3, response.etag);
    insert->bind(4, response.expires);
    insert->bind(5, response.modified);
    insert->bind(6, SystemClock::now());

    if (response.noContent) {
        insert->bind(7, nullptr);
        insert->bind(8, false);
    } else {
        insert->bindBlob(7, data.data(), data.size(), false);
        insert->bind(8, compressed);
    }

    insert->run();
    return true;
}

optional<Response> OfflineDatabase::getTile(const Resource::TileData& tile) {
    Statement accessedStmt = getStatement(
        "UPDATE tiles "
        "SET accessed       = ?1 "
        "WHERE url_template = ?2 "
        "  AND pixel_ratio  = ?3 "
        "  AND x            = ?4 "
        "  AND y            = ?5 "
        "  AND z            = ?6 ");

    accessedStmt->bind(1, SystemClock::now());
    accessedStmt->bind(2, tile.urlTemplate);
    accessedStmt->bind(3, tile.pixelRatio);
    accessedStmt->bind(4, tile.x);
    accessedStmt->bind(5, tile.y);
    accessedStmt->bind(6, tile.z);
    accessedStmt->run();

    Statement stmt = getStatement(
        //        0      1        2       3        4
        "SELECT etag, expires, modified, data, compressed "
        "FROM tiles "
        "WHERE url_template = ?1 "
        "  AND pixel_ratio  = ?2 "
        "  AND x            = ?3 "
        "  AND y            = ?4 "
        "  AND z            = ?5 ");

    stmt->bind(1, tile.urlTemplate);
    stmt->bind(2, tile.pixelRatio);
    stmt->bind(3, tile.x);
    stmt->bind(4, tile.y);
    stmt->bind(5, tile.z);

    if (!stmt->run()) {
        return {};
    }

    Response response;

    response.etag     = stmt->get<optional<std::string>>(0);
    response.expires  = stmt->get<optional<SystemTimePoint>>(1);
    response.modified = stmt->get<optional<SystemTimePoint>>(2);

    optional<std::string> data = stmt->get<optional<std::string>>(3);
    if (!data) {
        response.noContent = true;
    } else if (stmt->get<int>(4)) {
        response.data = std::make_shared<std::string>(util::decompress(*data));
    } else {
        response.data = std::make_shared<std::string>(*data);
    }

    return response;
}

bool OfflineDatabase::putTile(const Resource::TileData& tile,
                              const Response& response,
                              const std::string& data,
                              bool compressed) {
    if (response.notModified) {
        Statement update = getStatement(
            "UPDATE tiles "
            "SET accessed       = ?1, "
            "    expires        = ?2 "
            "WHERE url_template = ?3 "
            "  AND pixel_ratio  = ?4 "
            "  AND x            = ?5 "
            "  AND y            = ?6 "
            "  AND z            = ?7 ");

        update->bind(1, SystemClock::now());
        update->bind(2, response.expires);
        update->bind(3, tile.urlTemplate);
        update->bind(4, tile.pixelRatio);
        update->bind(5, tile.x);
        update->bind(6, tile.y);
        update->bind(7, tile.z);
        update->run();
        return false;
    }

    // We can't use REPLACE because it would change the id value.

    Statement update = getStatement(
        "UPDATE tiles "
        "SET modified       = ?1, "
        "    etag           = ?2, "
        "    expires        = ?3, "
        "    accessed       = ?4, "
        "    data           = ?5, "
        "    compressed     = ?6 "
        "WHERE url_template = ?7 "
        "  AND pixel_ratio  = ?8 "
        "  AND x            = ?9 "
        "  AND y            = ?10 "
        "  AND z            = ?11 ");

    update->bind(1, response.modified);
    update->bind(2, response.etag);
    update->bind(3, response.expires);
    update->bind(4, SystemClock::now());
    update->bind(7, tile.urlTemplate);
    update->bind(8, tile.pixelRatio);
    update->bind(9, tile.x);
    update->bind(10, tile.y);
    update->bind(11, tile.z);

    if (response.noContent) {
        update->bind(5, nullptr);
        update->bind(6, false);
    } else {
        update->bindBlob(5, data.data(), data.size(), false);
        update->bind(6, compressed);
    }

    update->run();
    if (db->changes() != 0) {
        return false;
    }

    Statement insert = getStatement(
        "INSERT INTO tiles (url_template, pixel_ratio, x,  y,  z,  modified,  etag,  expires,  accessed,  data, compressed) "
        "VALUES            (?1,           ?2,          ?3, ?4, ?5, ?6,        ?7,    ?8,       ?9,        ?10,  ?11) ");

    insert->bind(1, tile.urlTemplate);
    insert->bind(2, tile.pixelRatio);
    insert->bind(3, tile.x);
    insert->bind(4, tile.y);
    insert->bind(5, tile.z);
    insert->bind(6, response.modified);
    insert->bind(7, response.etag);
    insert->bind(8, response.expires);
    insert->bind(9, SystemClock::now());

    if (response.noContent) {
        insert->bind(10, nullptr);
        insert->bind(11, false);
    } else {
        insert->bindBlob(10, data.data(), data.size(), false);
        insert->bind(11, compressed);
    }

    insert->run();
    return true;
}

std::vector<OfflineRegion> OfflineDatabase::listRegions() {
    Statement stmt = getStatement(
        "SELECT id, definition, description FROM regions");

    std::vector<OfflineRegion> result;

    while (stmt->run()) {
        result.push_back(OfflineRegion(
            stmt->get<int64_t>(0),
            decodeOfflineRegionDefinition(stmt->get<std::string>(1)),
            stmt->get<std::vector<uint8_t>>(2)));
    }

    return result;
}

OfflineRegion OfflineDatabase::createRegion(const OfflineRegionDefinition& definition,
                                            const OfflineRegionMetadata& metadata) {
    Statement stmt = getStatement(
        "INSERT INTO regions (definition, description) "
        "VALUES              (?1,         ?2) ");

    stmt->bind(1, encodeOfflineRegionDefinition(definition));
    stmt->bindBlob(2, metadata);
    stmt->run();

    return OfflineRegion(db->lastInsertRowid(), definition, metadata);
}

void OfflineDatabase::deleteRegion(OfflineRegion&& region) {
    Statement stmt = getStatement(
        "DELETE FROM regions WHERE id = ?");

    stmt->bind(1, region.getID());
    stmt->run();

    evict(0);
}

optional<Response> OfflineDatabase::getRegionResource(int64_t regionID, const Resource& resource) {
    auto response = get(resource);

    if (response) {
        markUsed(regionID, resource);
    }

    return response;
}

uint64_t OfflineDatabase::putRegionResource(int64_t regionID, const Resource& resource, const Response& response) {
    uint64_t size = putInternal(resource, response, false).second;
    markUsed(regionID, resource);
    return size;
}

void OfflineDatabase::markUsed(int64_t regionID, const Resource& resource) {
    if (resource.kind == Resource::Kind::Tile) {
        Statement stmt = getStatement(
            "INSERT OR IGNORE INTO region_tiles (region_id, tile_id) "
            "SELECT                              ?1,        tiles.id "
            "FROM tiles "
            "WHERE url_template = ?2 "
            "  AND pixel_ratio  = ?3 "
            "  AND x            = ?4 "
            "  AND y            = ?5 "
            "  AND z            = ?6 ");

        const Resource::TileData& tile = *resource.tileData;
        stmt->bind(1, regionID);
        stmt->bind(2, tile.urlTemplate);
        stmt->bind(3, tile.pixelRatio);
        stmt->bind(4, tile.x);
        stmt->bind(5, tile.y);
        stmt->bind(6, tile.z);
        stmt->run();
    } else {
        Statement stmt = getStatement(
            "INSERT OR IGNORE INTO region_resources (region_id, resource_id) "
            "SELECT                                  ?1,        resources.id "
            "FROM resources "
            "WHERE resources.url = ?2 ");

        stmt->bind(1, regionID);
        stmt->bind(2, resource.url);
        stmt->run();
    }
}

OfflineRegionDefinition OfflineDatabase::getRegionDefinition(int64_t regionID) {
    Statement stmt = getStatement(
        "SELECT definition FROM regions WHERE id = ?1");

    stmt->bind(1, regionID);
    stmt->run();

    return decodeOfflineRegionDefinition(stmt->get<std::string>(0));
}

OfflineRegionStatus OfflineDatabase::getRegionCompletedStatus(int64_t regionID) {
    OfflineRegionStatus result;

    Statement stmt = getStatement(
        "SELECT COUNT(*), SUM(size) FROM ( "
        "  SELECT LENGTH(data) as size "
        "  FROM region_resources, resources "
        "  WHERE region_id = ?1 "
        "  AND resource_id = resources.id "
        "  UNION ALL "
        "  SELECT LENGTH(data) as size "
        "  FROM region_tiles, tiles "
        "  WHERE region_id = ?1 "
        "  AND tile_id = tiles.id "
        ") ");

    stmt->bind(1, regionID);
    stmt->run();

    result.completedResourceCount = stmt->get<int64_t>(0);
    result.completedResourceSize  = stmt->get<int64_t>(1);

    return result;
}

template <class T>
T OfflineDatabase::getPragma(const char * sql) {
    Statement stmt = getStatement(sql);
    stmt->run();
    return stmt->get<T>(0);
}

// Remove least-recently used resources and tiles until the used database size,
// as calculated by multiplying the number of in-use pages by the page size, is
// less than the maximum cache size. Returns false if this condition cannot be
// satisfied.
//
// SQLite database never shrinks in size unless we call VACCUM. We here
// are monitoring the soft limit (i.e. number of free pages in the file)
// and as it approaches to the hard limit (i.e. the actual file size) we
// delete an arbitrary number of old cache entries. The free pages approach saves
// us from calling VACCUM or keeping a running total, which can be costly.
bool OfflineDatabase::evict(uint64_t neededFreeSize) {
    uint64_t pageSize = getPragma<int64_t>("PRAGMA page_size");
    uint64_t pageCount = getPragma<int64_t>("PRAGMA page_count");

    if (pageSize * pageCount > maximumCacheSize) {
        Log::Warning(mbgl::Event::Database, "Current size is larger than the maximum size. Database won't get truncated.");
    }

    auto usedSize = [&] {
        return pageSize * (pageCount - getPragma<int64_t>("PRAGMA freelist_count"));
    };

    // The addition of pageSize is a fudge factor to account for non `data` column
    // size, and because pages can get fragmented on the database.
    while (usedSize() + neededFreeSize + pageSize > maximumCacheSize) {
        Statement stmt1 = getStatement(
            "DELETE FROM resources "
            "WHERE id IN ( "
            "  SELECT id FROM resources "
            "  LEFT JOIN region_resources "
            "  ON resource_id = resources.id "
            "  WHERE resource_id IS NULL "
            "  ORDER BY accessed ASC LIMIT ?1 "
            ") ");
        stmt1->bind(1, 50);
        stmt1->run();
        uint64_t changes1 = db->changes();

        Statement stmt2 = getStatement(
            "DELETE FROM tiles "
            "WHERE id IN ( "
            "  SELECT id FROM tiles "
            "  LEFT JOIN region_tiles "
            "  ON tile_id = tiles.id "
            "  WHERE tile_id IS NULL "
            "  ORDER BY accessed ASC LIMIT ?1 "
            ") ");
        stmt2->bind(1, 50);
        stmt2->run();
        uint64_t changes2 = db->changes();

        if (changes1 == 0 && changes2 == 0) {
            return false;
        }
    }

    return true;
}

} // namespace mbgl
