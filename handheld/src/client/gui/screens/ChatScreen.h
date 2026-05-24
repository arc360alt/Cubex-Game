#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__ChatScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__ChatScreen_H__

#include "../Screen.h"

class TextBox;

class ChatScreen: public Screen
{
	typedef Screen super;
public:
	ChatScreen();
	virtual ~ChatScreen();

	void init();
	void setupPositions();
	void tick();

	void render(int xm, int ym, float a);
	bool handleBackEvent(bool isDown);
	bool isPauseScreen() { return false; }
	bool shouldSaveOnOpen() { return false; }
	void keyPressed(int eventKey);
	void keyboardNewChar(char inputChar);
	void mouseClicked(int x, int y, int buttonNum);

	void buttonClicked(Button* button) {};

private:
	void sendMessage();

	TextBox* textBox;
	int caretTick;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__ChatScreen_H__*/
