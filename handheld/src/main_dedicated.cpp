#include <iostream>
#include "NinecraftApp.h"
#include "AppPlatform.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "world/level/LevelSettings.h"
#include "world/level/Level.h"
#include "server/ArgumentsSettings.h"
#include "platform/CThread.h"
#include "platform/time.h"
#include "SharedConstants.h"
#include "world/Difficulty.h"
#include "world/entity/player/Player.h"

#define MAIN_CLASS NinecraftApp
static App* g_app = 0;
static int g_exitCode = 0;
void signal_callback_handler(int signum) {
	std::cout << "Signum caught: " << signum << std::endl;
	if(signum == 2 || signum == 3){ // SIGINT ||  SIGQUIT

		if(g_app != 0) {
			g_app->quit();
		} else {
			exit(g_exitCode);
		}
	}
}

int main(int numArguments, char* pszArgs[]) {
	ArgumentsSettings aSettings(numArguments, pszArgs);
	if(aSettings.getShowHelp()) {
		ArgumentsSettings defaultSettings(0, NULL);
		printf("Minecraft Pockect Edition Server %s\n", Common::getGameVersionString("").c_str());
		printf("-------------------------------------------------------\n");
		printf("--cachepath - Path to where the server can store temp stuff (not sure if this is used) [default: \"%s\"]\n", defaultSettings.getCachePath().c_str());
		printf("--config - Path to key=value server config file [default: \"%s\"]\n", defaultSettings.getConfigPath().c_str());
		printf("--externalpath - The path to the place where the server should store the levels. [default: \"%s\"]\n", defaultSettings.getExternalPath().c_str());
		printf("--levelname - The name of the server [default: \"%s\"]\n", defaultSettings.getLevelName().c_str());
		printf("--leveldir - The name of the server [default: \"%s\"]\n", defaultSettings.getLevelDir().c_str());
		printf("--help - Shows this message.\n");
		printf("--port - The port to run the server on. [default: %d]\n", defaultSettings.getPort());
		printf("--gamemode - survival|creative (or 0|1) [default: survival]\n");
		printf("--worldtype - old|infinite (or 0|1) [default: old]\n");
		printf("--difficulty - peaceful|easy|normal|hard (or 0|1|2|3) [default: normal]\n");
		printf("--serverkey - The key that the server should use for API calls. [default: \"%s\"]\n", defaultSettings.getServerKey().c_str());
		printf("-------------------------------------------------------\n");
		return 0;
	}
	printf("Config: %s\n", aSettings.getConfigPath().c_str());
	printf("External Path: %s\n", aSettings.getExternalPath().c_str());
	printf("Level Dir: %s\n", aSettings.getLevelDir().c_str());
	printf("Level Name: %s\n", aSettings.getLevelName().c_str());
	printf("World Type: %s\n", aSettings.getWorldType() == WorldType::Infinite ? "infinite" : "old");
	AppContext appContext;
	appContext.platform = new AppPlatform();
	App* app = new MAIN_CLASS();
	signal(SIGINT, signal_callback_handler);
	g_app = app;
	((MAIN_CLASS*)g_app)->externalStoragePath = aSettings.getExternalPath();
	((MAIN_CLASS*)g_app)->externalCacheStoragePath = aSettings.getCachePath();

	g_app->init(appContext);
	const int serverGameType = LevelSettings::validateGameType(aSettings.getGameType());
	const int serverWorldType = (aSettings.getWorldType() == WorldType::Infinite) ? WorldType::Infinite : WorldType::Old;
	const int serverDifficulty = aSettings.getDifficulty();
	LevelSettings settings(getEpochTimeS(), serverGameType, serverWorldType);
	float startTime = getTimeS();
	((MAIN_CLASS*)g_app)->selectLevel(aSettings.getLevelDir(), aSettings.getLevelName(),  settings);
	if (((MAIN_CLASS*)g_app)->level) {
		((MAIN_CLASS*)g_app)->level->getLevelData()->setGameType(serverGameType);
		((MAIN_CLASS*)g_app)->level->getLevelData()->setWorldType(serverWorldType);
		((MAIN_CLASS*)g_app)->level->getLevelData()->setSpawnMobs(serverGameType == GameType::Survival);
		((MAIN_CLASS*)g_app)->level->difficulty = serverDifficulty;
		((MAIN_CLASS*)g_app)->options.difficulty = serverDifficulty;
		((MAIN_CLASS*)g_app)->setIsCreativeMode(serverGameType == GameType::Creative);
	}
	((MAIN_CLASS*)g_app)->hostMultiplayer(aSettings.getPort());

	std::cout << "Level has been generated in " << getTimeS() - startTime << std::endl;
	((MAIN_CLASS*)g_app)->level->saveLevelData();
	std::cout << "Level has been saved!" << std::endl;
	while(!app->wantToQuit()) {
		app->update();
		CThread::sleep(1);
	}
	if (((MAIN_CLASS*)g_app)->level && ((MAIN_CLASS*)g_app)->level->getLevelStorage()) {
		const PlayerList& players = ((MAIN_CLASS*)g_app)->level->players;
		for (unsigned int i = 0; i < players.size(); ++i)
			((MAIN_CLASS*)g_app)->level->getLevelStorage()->saveServerPlayer(players[i]);
	}
	((MAIN_CLASS*)g_app)->level->saveLevelData();
	delete app;
	appContext.platform->finish();
	delete appContext.platform;
	std::cout << "Quit correctly" << std::endl;
	return g_exitCode;
}