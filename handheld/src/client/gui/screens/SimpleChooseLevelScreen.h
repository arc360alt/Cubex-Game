#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__DemoChooseLevelScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__DemoChooseLevelScreen_H__

#include "ChooseLevelScreen.h"
#include "../components/Button.h"
#include "../components/TextBox.h"

class SimpleChooseLevelScreen: public ChooseLevelScreen
{
public:
	SimpleChooseLevelScreen(const std::string& levelName);

	virtual ~SimpleChooseLevelScreen();

	void init();
	void setupPositions();
	void render(int xm, int ym, float a);
	void buttonClicked(Button* button);
	void keyboardNewChar(char inputChar);
	void keyPressed(int key);
	bool handleBackEvent(bool isDown);

private:
	// Header / nav
	Touch::THeader* bTitle;
	Touch::TButton* bBack;
	Touch::TButton* bNameContinue;

	// Step 1: game mode
	Touch::TButton* bCreative;
	Touch::TButton* bSurvival;

	// Step 2: world type
	Touch::TButton* bOldWorld;
	Touch::TButton* bInfiniteWorld;
	TextBox* tbWorldName;

	bool hasChosen;
	int step; // 0: name, 1: game mode, 2: world type
	int chosenGameType; // -1 until chosen

	std::string levelName;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__DemoChooseLevelScreen_H__*/
