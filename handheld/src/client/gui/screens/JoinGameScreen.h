#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__JoinGameScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__JoinGameScreen_H__

#include "../Screen.h"
#include "../components/Button.h"
#include "../components/SmallButton.h"
#include "../components/ScrolledSelectionList.h"
#include "../../Minecraft.h"
#include "../../../network/RakNetInstance.h"
#include <cstdio>


class JoinGameScreen;

class AvailableGamesList : public ScrolledSelectionList
{
	int selectedItem;
	int listWidth;
	ServerList copiedServerList;

	friend class JoinGameScreen;

public:

	AvailableGamesList(Minecraft* _minecraft, int _width, int _height)
	:	ScrolledSelectionList(_minecraft, _width, _height, 24, _height - 30, 28),
		selectedItem(-1),
		listWidth(_width)
	{
	}

protected:

	virtual int getNumberOfItems() { return (int)copiedServerList.size(); }

	virtual void selectItem(int item, bool doubleClick) { selectedItem = item; }
	virtual bool isSelectedItem(int item) { return item == selectedItem; }

	virtual void renderBackground() {}
	virtual void renderItem(int i, int x, int y, int h, Tesselator& t)
	{
		if (isSelectedItem(i)) {
			const int left = listWidth / 2 - (92 + 16 + 2);
			const int right = listWidth / 2 + (92 + 16 + 2);
			fill(left, y, right, y + h, 0x60407090);
			fill(left, y, right, y + 1, 0xffd8ecff);
			fill(left, y + h - 1, right, y + h, 0xffd8ecff);
			fill(left, y, left + 1, y + h, 0xffd8ecff);
			fill(right - 1, y, right, y + h, 0xffd8ecff);
		}

		const PingedCompatibleServer& s = copiedServerList[i];
		unsigned int color = s.isSpecial? 0xff00b0 : 0xffffa0;
		drawString(minecraft->font, s.name.C_String(), x, y + 2, color);
		drawString(minecraft->font, s.address.ToString(false), x, y + 16, 0xffffa0);
		if (s.latencyMs > 0) {
			char pingText[16];
			std::snprintf(pingText, sizeof(pingText), "%ums", (unsigned int)s.latencyMs);
			drawString(minecraft->font, pingText, x + 180, y + 2, 0xffd060);
		}
	}
};

#include "../components/TextBox.h"
#include <vector>

class JoinGameScreen: public Screen
{
public:
	JoinGameScreen();
	virtual ~JoinGameScreen();

	void init();
	void setupPositions();

	virtual bool handleBackEvent(bool isDown);

	virtual bool isIndexValid(int index);

	virtual void tick();

	void render(int xm, int ym, float a);

	void buttonClicked(Button* button);
	virtual void keyboardNewChar(char inputChar);
	virtual void keyPressed(int key);

	bool isInGameScreen();
private:
	Button bJoin;
	Button bBack;
	Button bAddServer;
	Button bAddConnect;
	Button bRemoveServer;
	Button bAddCancel;
	TextBox* txtServerAddress;
	TextBox* txtServerPort;
	AvailableGamesList* gamesList;
	bool addServerMode;
	std::string addServerError;
	std::vector<PingedCompatibleServer> customServers;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__JoinGameScreen_H__*/
