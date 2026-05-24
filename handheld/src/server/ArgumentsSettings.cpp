#include "ArgumentsSettings.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <algorithm>
#include "../world/level/LevelSettings.h"
#include "../world/Difficulty.h"

static std::string trim(const std::string& s) {
	std::string::size_type b = 0;
	while (b < s.size() && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) b++;
	std::string::size_type e = s.size();
	while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) e--;
	return s.substr(b, e - b);
}

static std::string lowerCopy(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}

static int parseGameType(const std::string& value) {
	const std::string v = lowerCopy(trim(value));
	if (v == "creative" || v == "1") return GameType::Creative;
	if (v == "survival" || v == "0") return GameType::Survival;
	return GameType::Survival;
}

static int parseWorldType(const std::string& value) {
	const std::string v = lowerCopy(trim(value));
	if (v == "infinite" || v == "1") return WorldType::Infinite;
	if (v == "old" || v == "finite" || v == "0") return WorldType::Old;
	return WorldType::Old;
}

static int parseDifficulty(const std::string& value) {
	const std::string v = lowerCopy(trim(value));
	if (v == "peaceful" || v == "0") return Difficulty::PEACEFUL;
	if (v == "easy" || v == "1") return Difficulty::EASY;
	if (v == "normal" || v == "2") return Difficulty::NORMAL;
	if (v == "hard" || v == "3") return Difficulty::HARD;
	return Difficulty::NORMAL;
}

ArgumentsSettings::ArgumentsSettings(int numArguments, char** arguments)
: externalPath("."), levelName("level"), showHelp(false), port(19132), serverKey(""), cachePath("."), levelDir("level"), configPath("server.properties"), gameType(GameType::Survival), worldType(WorldType::Old), difficulty(Difficulty::NORMAL) {
	for(int a = 0; a < numArguments; ++a) {
		if(strcmp(arguments[a], "--config") == 0 && a + 1 < numArguments) {
			configPath = std::string(arguments[a + 1]);
			a++;
		}
	}

	loadConfigFile(configPath);

	for(int a = 0; a < numArguments; ++a) {
		if(strcmp(arguments[a], "--help") == 0) {
			showHelp = true;
		} else if(strcmp(arguments[a], "--config") == 0) {
			if(a + 1 < numArguments) {
				configPath = std::string(arguments[a + 1]);
				a++;
			}
		} else if(strcmp(arguments[a], "--externalpath") == 0) {
			if(a + 1 < numArguments) {
				externalPath = std::string(arguments[a+1]);
				a++; // Skip the next argument since it's part of this one.
			}
		} else if(strcmp(arguments[a], "--levelname") == 0) {
			if(a + 1 < numArguments) {
				levelName = std::string(arguments[a+1]);
				a++; // Skip the next argument since it's part of this one.
			}
		} else if(strcmp(arguments[a], "--leveldir") == 0) {
			if(a + 1 < numArguments) {
				levelDir = std::string(arguments[a+1]);
				a++; // Skip the next argument since it's part of this one.
			}
		} else if(strcmp(arguments[a], "--port") == 0) {
			if(a + 1 < numArguments) {
				port = atoi(arguments[a+1]);
				a++; // Skip the next argument since it's part of this one.
			}
		} else if(strcmp(arguments[a], "--serverkey") == 0) {
			if(a + 1 < numArguments) {
				serverKey = std::string(arguments[a+1]);
				a++; // Skip the next argument since it's part of this one.
			}
		} else if(strcmp(arguments[a], "--cachepath") == 0) {
			if(a + 1 < numArguments) {
				cachePath = std::string(arguments[a+1]);
				a++;
			}
		} else if(strcmp(arguments[a], "--gamemode") == 0) {
			if(a + 1 < numArguments) {
				gameType = parseGameType(arguments[a + 1]);
				a++;
			}
		} else if(strcmp(arguments[a], "--worldtype") == 0) {
			if(a + 1 < numArguments) {
				worldType = parseWorldType(arguments[a + 1]);
				a++;
			}
		} else if(strcmp(arguments[a], "--difficulty") == 0) {
			if(a + 1 < numArguments) {
				difficulty = parseDifficulty(arguments[a + 1]);
				a++;
			}
		}
	}
}

void ArgumentsSettings::loadConfigFile(const std::string& path) {
	std::ifstream in(path.c_str());
	if (!in) return;

	std::string line;
	while (std::getline(in, line)) {
		std::string l = trim(line);
		if (l.empty() || l[0] == '#') continue;

		std::string::size_type eq = l.find('=');
		if (eq == std::string::npos) continue;

		std::string key = lowerCopy(trim(l.substr(0, eq)));
		std::string value = trim(l.substr(eq + 1));

		if (key == "externalpath") externalPath = value;
		else if (key == "levelname") levelName = value;
		else if (key == "leveldir") levelDir = value;
		else if (key == "port") port = atoi(value.c_str());
		else if (key == "serverkey") serverKey = value;
		else if (key == "cachepath") cachePath = value;
		else if (key == "gamemode") gameType = parseGameType(value);
		else if (key == "worldtype") worldType = parseWorldType(value);
		else if (key == "difficulty") difficulty = parseDifficulty(value);
	}
}
std::string ArgumentsSettings::getExternalPath() {
	return externalPath;
}
std::string ArgumentsSettings::getLevelName() {
	return levelName;
}
std::string ArgumentsSettings::getServerKey() {
	return serverKey;
}
std::string ArgumentsSettings::getCachePath() {
	return cachePath;
}
std::string ArgumentsSettings::getLevelDir() {
	return levelDir;
}
bool ArgumentsSettings::getShowHelp() {
	return showHelp;
}
int ArgumentsSettings::getPort() {
	return port;
}

int ArgumentsSettings::getGameType() {
	return gameType;
}

int ArgumentsSettings::getWorldType() {
	return worldType;
}

int ArgumentsSettings::getDifficulty() {
	return difficulty;
}

std::string ArgumentsSettings::getConfigPath() {
	return configPath;
}
