#ifndef NET_MINECRAFT_WORLD_LEVEL__ArgumentsSettings_H__
#define NET_MINECRAFT_WORLD_LEVEL__ArgumentsSettings_H__
#include <string>
class ArgumentsSettings {
public:
	ArgumentsSettings(int numArguments, char** arguments);
	std::string getExternalPath();
	std::string getLevelName();
	std::string getServerKey();
	std::string getCachePath();
	std::string getLevelDir();
	bool getShowHelp();
	int getPort();
	int getGameType();
	int getWorldType();
	int getDifficulty();
	std::string getConfigPath();
	std::string getAccountsDbPath();
private:
	void loadConfigFile(const std::string& path);
	std::string cachePath;
	std::string externalPath;
	std::string levelName;
	std::string levelDir;
	std::string serverKey;
	std::string configPath;
	std::string accountsDbPath;
	bool showHelp;
	int port;
	int gameType;
	int worldType;
	int difficulty;
};

#endif
