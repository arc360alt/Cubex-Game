#ifndef NET_MINECRAFT_SERVER__AccountAuth_H__
#define NET_MINECRAFT_SERVER__AccountAuth_H__

// Only the dedicated server links sqlite3 (see dedicated_server/CMakeLists.txt),
// so this header must only ever be included under STANDALONE_SERVER.
#include <sqlite3.h>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <random>

// Verifies a login against the account database created/maintained by
// backend/server.py (see backend/accounts.db). This talks to the same SQLite
// file directly instead of going over HTTP, since the game server and the
// account backend normally run on the same machine.
class AccountAuth {
public:
	// Returns true if `token` belongs to an account whose username matches
	// `claimedName` (case-insensitively). On success, `outCanonicalName` is set
	// to the exact-case username stored in the account record (used as the
	// player's name from then on so player-data files stay consistently tied
	// to one account regardless of what case the client sent), and `outUuid`
	// is set to that account's stable uuid. Accounts created before the uuid
	// column existed are lazily backfilled with a fresh one right here, so
	// simply joining a server is enough to get one even if the player never
	// revisits the account backend directly.
	static bool verify(const std::string& dbPath, const std::string& token,
	                    const std::string& claimedName, std::string& outCanonicalName,
	                    std::string& outUuid)
	{
		if (dbPath.empty() || token.empty())
			return false;

		sqlite3* db = NULL;
		// Read-write: may need to backfill a uuid (see above).
		if (sqlite3_open_v2(dbPath.c_str(), &db, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK) {
			if (db) sqlite3_close(db);
			return false;
		}

		static const char* SQL = "SELECT id, username, uuid FROM accounts WHERE token = ?";
		sqlite3_stmt* stmt = NULL;
		bool ok = false;
		int accountId = -1;

		if (sqlite3_prepare_v2(db, SQL, -1, &stmt, NULL) == SQLITE_OK) {
			sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				accountId = sqlite3_column_int(stmt, 0);
				const unsigned char* nameText = sqlite3_column_text(stmt, 1);
				const unsigned char* uuidText = sqlite3_column_text(stmt, 2);
				std::string dbUsername = nameText ? reinterpret_cast<const char*>(nameText) : "";
				std::string dbUuid = uuidText ? reinterpret_cast<const char*>(uuidText) : "";

				if (!dbUsername.empty() && lowerCopy(dbUsername) == lowerCopy(claimedName)) {
					outCanonicalName = dbUsername;
					outUuid = dbUuid;
					ok = true;
				}
			}
			sqlite3_finalize(stmt);

			if (ok && outUuid.empty()) {
				outUuid = generateUuidV4();
				backfillUuid(db, accountId, outUuid);
			}
		}

		sqlite3_close(db);
		return ok;
	}

private:
	static std::string lowerCopy(const std::string& s) {
		std::string out(s);
		std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::tolower(c); });
		return out;
	}

	static void backfillUuid(sqlite3* db, int accountId, const std::string& uuid) {
		if (accountId < 0) return;
		sqlite3_stmt* stmt = NULL;
		if (sqlite3_prepare_v2(db, "UPDATE accounts SET uuid = ? WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
			sqlite3_bind_text(stmt, 1, uuid.c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_bind_int(stmt, 2, accountId);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	static std::string generateUuidV4() {
		std::random_device rd;
		std::mt19937_64 gen(rd());
		std::uniform_int_distribution<uint64_t> dist;
		uint64_t hi = dist(gen);
		uint64_t lo = dist(gen);
		// Set version (4) and variant bits per RFC 4122.
		hi = (hi & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
		lo = (lo & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;
		char buf[37];
		snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%04x-%012llx",
		         (unsigned int)(hi >> 32), (unsigned int)((hi >> 16) & 0xFFFF), (unsigned int)(hi & 0xFFFF),
		         (unsigned int)(lo >> 48), (unsigned long long)(lo & 0xFFFFFFFFFFFFULL));
		return std::string(buf);
	}
};

#endif /*NET_MINECRAFT_SERVER__AccountAuth_H__*/
