#include "TouchJoinGameScreen.h"
#include "../StartMenuScreen.h"
#include "../ProgressScreen.h"
#include "../../Font.h"
#include "../../../Minecraft.h"
#include "../../../renderer/Textures.h"
#include <cstring>

namespace Touch {

//
// Games list
//

void AvailableGamesList::selectStart( int item) {
	startSelected = item;
}

void AvailableGamesList::selectCancel() {
	startSelected = -1;
}

void AvailableGamesList::selectItem( int item, bool doubleClick ) {
	LOGI("selected an item! %d\n", item);
	selectedItem = item;
}

void AvailableGamesList::renderItem( int i, int x, int y, int h, Tesselator& t )
{
	if (startSelected == i && Multitouch::getFirstActivePointerIdEx() >= 0) {
		fill((int)x0, y, (int)x1, y+h, 0x809E684F);
	}
	if (selectedItem == i) {
		fill((int)x0, y, (int)x1, y + h, 0x60407090);
		fill((int)x0, y, (int)x1, y + 1, 0xffd8ecff);
		fill((int)x0, y + h - 1, (int)x1, y + h, 0xffd8ecff);
		fill((int)x0, y, (int)x0 + 1, y + h, 0xffd8ecff);
		fill((int)x1 - 1, y, (int)x1, y + h, 0xffd8ecff);
	}

	//static int colors[2] = {0xffffb0, 0xcccc90};
	const PingedCompatibleServer& s = copiedServerList[i];
	unsigned int color  = s.isSpecial? 0x6090a0 : 0xffffb0;
	unsigned int color2 = 0xffffa0;//colors[i&1];

	int xx1 = (int)x0 + 24;
	int xx2 = xx1;

	if (s.isSpecial) {
		xx1 += 50;

		glEnable2(GL_TEXTURE_2D);
        glColor4f2(1,1,1,1);
        glEnable2(GL_BLEND);
		minecraft->textures->loadAndBindTexture("gui/badge/minecon140.png");
		blit(xx2, y + 6, 0, 0, 37, 8, 140, 240);
	}

	drawString(minecraft->font, s.name.C_String(), xx1, y + 4 + 2, color);
	drawString(minecraft->font, s.address.ToString(false), xx2, y + 18, color2);
	if (s.latencyMs > 0) {
		char pingText[16];
		std::snprintf(pingText, sizeof(pingText), "%ums", (unsigned int)s.latencyMs);
		drawString(minecraft->font, pingText, (int)x1 - 44, y + 4 + 2, 0xffd060);
	}

	/*
	drawString(minecraft->font, copiedServerList[i].name.C_String(), (int)x0 + 24, y + 4, color);
	drawString(minecraft->font, copiedServerList[i].address.ToString(false), (int)x0 + 24, y + 18, color);
	*/
}


//
// Join Game screen
//
JoinGameScreen::JoinGameScreen()
:	bJoin(  2, "Join Game"),
	bBack(  3, "Back"),
	bHeader(0, ""),
	bAddServer(4, "Add Server"),
	bAddConnect(5, "Add"),
	bRemoveServer(7, "Delete"),
	bAddCancel(6, "Cancel"),
	txtServerAddress(NULL),
	txtServerPort(NULL),
	gamesList(NULL),
	addServerMode(false),
	addServerError("")
{
	bJoin.active = false;
	bAddConnect.active = false;
	bAddCancel.active = false;
}

JoinGameScreen::~JoinGameScreen()
{
	delete gamesList;
	if (txtServerAddress) delete txtServerAddress;
	if (txtServerPort) delete txtServerPort;
}

void JoinGameScreen::init()
{
	buttons.push_back(&bAddServer);
	buttons.push_back(&bBack);
	buttons.push_back(&bHeader);
	buttons.push_back(&bJoin);
	buttons.push_back(&bAddConnect);
	buttons.push_back(&bRemoveServer);
	buttons.push_back(&bAddCancel);

	bAddConnect.visible = false;
	bRemoveServer.visible = false;
	bAddCancel.visible = false;

	minecraft->raknetInstance->clearServerList();
	gamesList = new AvailableGamesList(minecraft, width, height);
	customServers.clear();

	// Text inputs for server entry
	txtServerAddress = new TextBox(10, "");
	txtServerAddress->w = width - 20;
	txtServerAddress->h = 24;
	textBoxes.push_back(txtServerAddress);

	txtServerPort = new TextBox(11, "19132");
	txtServerPort->w = 100;
	txtServerPort->h = 24;
	textBoxes.push_back(txtServerPort);

	addServerMode = false;
	addServerError = "";

#ifdef ANDROID
	tabButtons.push_back(&bBack);
	tabButtons.push_back(&bAddServer);
	tabButtons.push_back(&bJoin);
	tabButtons.push_back(&bAddConnect);
	tabButtons.push_back(&bAddCancel);
#endif
}

void JoinGameScreen::setupPositions() {
	int yBase = 0;
	int spacing = 4;
	int buttonCount = 5; // back, add, join, remove, header
	int buttonWidth = (width - spacing * (buttonCount - 1)) / buttonCount;
	if (buttonWidth < 60) buttonWidth = 60;

	int x = 0;
	bBack.x = x;
	bBack.y = yBase;
	bBack.width = buttonWidth;
	x += buttonWidth + spacing;

	bAddServer.x = x;
	bAddServer.y = yBase;
	bAddServer.width = buttonWidth;
	x += buttonWidth + spacing;

	bJoin.x = x;
	bJoin.y = yBase;
	bJoin.width = buttonWidth;
	x += buttonWidth + spacing;

	bRemoveServer.x = x;
	bRemoveServer.y = yBase;
	bRemoveServer.width = buttonWidth;
	x += buttonWidth + spacing;

	bHeader.x = x;
	bHeader.y = yBase;
	bHeader.width = width - x;

	// add-server popup position
	if (txtServerAddress) {
		int fieldX = 10;
		int fieldY = 40;
		txtServerAddress->x = fieldX;
		txtServerAddress->y = fieldY;
	}
	if (txtServerPort) {
		int fieldX = 10;
		int fieldY = 70;
		txtServerPort->x = fieldX;
		txtServerPort->y = fieldY;
	}

	int yBottom = height - 28;
	bAddConnect.x = width / 2 - bAddConnect.width - 2;
	bAddConnect.y = yBottom;
	bAddCancel.x = width / 2 + 2;
	bAddCancel.y = yBottom;
}

void JoinGameScreen::buttonClicked(Button* button)
{
	if (addServerMode) {
		if (button == &bAddConnect) {
			std::string host = txtServerAddress? txtServerAddress->text : std::string();
			std::string portStr = txtServerPort? txtServerPort->text : std::string();
			int port = atoi(portStr.c_str());
			if (host.empty()) {
				addServerError = "Enter server address";
				return;
			}
			if (port <= 0 || port > 65535) {
				addServerError = "Port must be 1-65535";
				return;
			}
			addServerError.clear();

			PingedCompatibleServer newServer;
			newServer.name = RakNet::RakString("Custom: %s:%d", host.c_str(), port);
			newServer.address = RakNet::SystemAddress(host.c_str(), port);
			newServer.lastSeenTime = 0;
			newServer.latencyMs = 0;
			newServer.isSpecial = false;

			bool exists = false;
			for (size_t i = 0; i < customServers.size(); ++i) {
				if (customServers[i].address == newServer.address) {
					exists = true;
					customServers[i].name = newServer.name;
					break;
				}
			}
			if (!exists) {
				customServers.push_back(newServer);
			}

			addServerMode = false;
			bAddConnect.visible = false;
			bAddCancel.visible = false;
			bAddServer.active = true;
			bJoin.active = true;
			minecraft->platform()->hideKeyboard();
			return;
		}

		if (button == &bAddCancel || button == &bBack) {
			addServerMode = false;
			bAddServer.active = true;
			bJoin.active = true;
			bAddConnect.visible = false;
			bAddCancel.visible = false;
			addServerError.clear();
			minecraft->platform()->hideKeyboard();
			return;
		}
	}
	else {
		if (button == &bAddServer) {
			addServerMode = true;
			bAddConnect.visible = true;
			bAddCancel.visible = true;
			bJoin.active = false;
			bAddServer.active = false;
			bAddConnect.active = true;
			bAddCancel.active = true;
			if (txtServerAddress) txtServerAddress->setFocus(minecraft);
			return;
		}

		if (button == &bJoin) {
			if (isIndexValid(gamesList->selectedItem)) {
				PingedCompatibleServer selectedServer = gamesList->copiedServerList[gamesList->selectedItem];
				minecraft->joinMultiplayer(selectedServer);
				bJoin.active = false;
				bBack.active = false;
				minecraft->setScreen(new ProgressScreen());
			}
			return;
		}

		if (button == &bRemoveServer) {
			if (isIndexValid(gamesList->selectedItem)) {
				PingedCompatibleServer selectedServer = gamesList->copiedServerList[gamesList->selectedItem];
				for (auto it = customServers.begin(); it != customServers.end(); ++it) {
					if (it->address == selectedServer.address) {
						customServers.erase(it);
						break;
					}
				}
				return;
			}
		}

		if (button == &bBack) {
			minecraft->cancelLocateMultiplayer();
			minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
			return;
		}
	}
}

bool JoinGameScreen::handleBackEvent(bool isDown)
{
	if (!isDown)
	{
		if (addServerMode) {
			addServerMode = false;
			bAddConnect.visible = false;
			bAddCancel.visible = false;
			bAddServer.active = true;
			bJoin.active = true;
			addServerError.clear();
			minecraft->platform()->hideKeyboard();
			return true;
		}
		minecraft->cancelLocateMultiplayer();
		minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
	}
	return true;
}

void JoinGameScreen::keyboardNewChar(char inputChar)
{
	if (!addServerMode)
		return;

	if (txtServerAddress && txtServerAddress->focused) {
		if (inputChar >= 32 && inputChar < 127)
			txtServerAddress->insertChar(inputChar, 64);
		return;
	}
	if (txtServerPort && txtServerPort->focused) {
		if (inputChar >= '0' && inputChar <= '9')
			txtServerPort->insertChar(inputChar, 5);
		return;
	}
}

void JoinGameScreen::keyPressed(int key)
{
	if (!addServerMode) {
		Screen::keyPressed(key);
		return;
	}

	if (txtServerAddress && txtServerAddress->focused && txtServerAddress->handleEditKey(key))
		return;
	if (txtServerPort && txtServerPort->focused && txtServerPort->handleEditKey(key))
		return;
	if (key == Keyboard::KEY_RETURN) {
		buttonClicked(&bAddConnect);
		return;
	}
	// still allow keyboard navigation for buttons
	Screen::keyPressed(key);
}

bool JoinGameScreen::isIndexValid( int index )
{
	return gamesList && index >= 0 && index < gamesList->getNumberOfItems();
}

void JoinGameScreen::tick()
{
	if (addServerMode) {
		bAddConnect.active = txtServerAddress && !txtServerAddress->text.empty() && txtServerPort && !txtServerPort->text.empty();
		bAddConnect.visible = true;
		bAddCancel.visible = true;
		bRemoveServer.visible = false;
		bAddServer.visible = false;
		bJoin.visible = false;
		return;
	}

	bAddServer.visible = true;
	bAddConnect.visible = false;
	bAddCancel.visible = false;
	bJoin.visible = true;

	const ServerList& orgServerList = minecraft->raknetInstance->getServerList();
	ServerList mergedList = customServers;

	for (unsigned int i = 0; i < orgServerList.size(); ++i) {
		if (orgServerList[i].name.GetLength() > 0) {
			bool found = false;
			for (unsigned int j = 0; j < mergedList.size(); ++j) {
				if (mergedList[j].address == orgServerList[i].address) {
					mergedList[j] = orgServerList[i];
					found = true;
					break;
				}
			}
			if (!found) {
				mergedList.push_back(orgServerList[i]);
			}
		}
	}

	if (mergedList.size() != gamesList->copiedServerList.size())
	{
		PingedCompatibleServer selectedServer;
		bool hasSelection = false;
		if (isIndexValid(gamesList->selectedItem))
		{
			selectedServer = gamesList->copiedServerList[gamesList->selectedItem];
			hasSelection = true;
		}

		gamesList->copiedServerList = mergedList;
		gamesList->selectItem(-1, false);

		if (hasSelection)
		{
			for (unsigned int i = 0; i < gamesList->copiedServerList.size(); i++)
			{
				if (gamesList->copiedServerList[i].address == selectedServer.address)
				{
					gamesList->selectItem(i, false);
					break;
				}
			}
		}
	} else {
		for (int i = (int)gamesList->copiedServerList.size()-1; i >= 0 ; --i) {
			for (int j = 0; j < (int) mergedList.size(); ++j)
				if (mergedList[j].address == gamesList->copiedServerList[i].address)
					gamesList->copiedServerList[i] = mergedList[j];
		}
	}

	bJoin.active = isIndexValid(gamesList->selectedItem);
	if (isIndexValid(gamesList->selectedItem)) {
		const char* serverName = gamesList->copiedServerList[gamesList->selectedItem].name.C_String();
		if (serverName && strncmp(serverName, "Custom:", 7) == 0) {
			bRemoveServer.visible = true;
			bRemoveServer.active = true;
		} else {
			bRemoveServer.visible = false;
			bRemoveServer.active = false;
		}
	} else {
		bRemoveServer.visible = false;
		bRemoveServer.active = false;
	}
}

void JoinGameScreen::render( int xm, int ym, float a )
{
	if (addServerMode) {
		renderBackground();
		// draw add-server panel
		fill(8, 24, width - 8, 120, 0xaa000000);
		minecraft->font->drawShadow("Add Server", 20, 30, 0xffffffff);
		minecraft->font->drawShadow("Address:", 20, 55, 0xffffffff);
		if (txtServerAddress) txtServerAddress->render(minecraft, xm, ym);
		minecraft->font->drawShadow("Port:", 20, 85, 0xffffffff);
		if (txtServerPort) txtServerPort->render(minecraft, xm, ym);
		if (!addServerError.empty()) {
			minecraft->font->drawShadow(addServerError, 20, 115, 0xffff4444);
		}
		Screen::render(xm, ym, a);
		return;
	}

	bool hasNetwork = minecraft->platform()->isNetworkEnabled(true);
#ifdef WIN32
	hasNetwork = hasNetwork && !GetAsyncKeyState(VK_TAB);
#endif

	renderBackground();
	if (hasNetwork) gamesList->render(xm, ym, a);
	else gamesList->renderDirtBackground();
	Screen::render(xm, ym, a);

	const int baseX = bHeader.x + bHeader.width / 2;

	if (hasNetwork) {
		std::string s = "Scanning for WiFi Games...";
		drawCenteredString(minecraft->font, s, baseX, 8, 0xffffffff);

		const int textWidth = minecraft->font->width(s);
		const int spinnerX = baseX + textWidth / 2 + 6;

		static const char* spinnerTexts[] = {"-", "\\", "|", "/"};
		int n = ((int)(5.5f * getTimeS()) % 4);
		drawCenteredString(minecraft->font, spinnerTexts[n], spinnerX, 8, 0xffffffff);
	} else {
		drawCenteredString(minecraft->font, "WiFi is disabled", baseX, 8, 0xffffffff);
	}
}

bool JoinGameScreen::isInGameScreen() { return false; }

};
