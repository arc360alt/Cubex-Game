#include "SimpleChooseLevelScreen.h"
#include "ProgressScreen.h"
#include "ScreenChooser.h"
#include "../../Minecraft.h"
#include "../../../world/level/LevelSettings.h"
#include "../../../platform/time.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../platform/input/Mouse.h"
#include "../../../util/StringUtils.h"

static const char ILLEGAL_FILE_CHARACTERS[] = {
	'/', '\n', '\r', '\t', '\0', '\f', '`', '?', '*', '\\', '<', '>', '|', '"', ':'
};

SimpleChooseLevelScreen::SimpleChooseLevelScreen(const std::string& levelName)
:	bTitle(0),
	bBack(0),
	bNameContinue(0),
	bCreative(0),
	bSurvival(0),
	bOldWorld(0),
	bInfiniteWorld(0),
	tbWorldName(0),
	tbSeed(0),
	levelName(levelName),
	seedText(""),
	hasChosen(false),
	step(0),
	chosenGameType(-1)
{
}

SimpleChooseLevelScreen::~SimpleChooseLevelScreen()
{
	delete bTitle;
	delete bBack;
	delete bNameContinue;
	delete bCreative;
	delete bSurvival;
	delete bOldWorld;
	delete bInfiniteWorld;
	delete tbWorldName;
	delete tbSeed;
}

void SimpleChooseLevelScreen::init()
{
	bTitle        = new Touch::THeader(0, "Game Mode");
	bBack         = new Touch::TButton(3, "Back");
	bNameContinue = new Touch::TButton(6, "Continue");
	bCreative     = new Touch::TButton(1, "Creative");
	bSurvival     = new Touch::TButton(2, "Survival");
	bOldWorld     = new Touch::TButton(4, "Old World");
	bInfiniteWorld= new Touch::TButton(5, "Infinite");
	tbWorldName   = new TextBox(10, levelName);
	tbWorldName->w = 220;
	tbWorldName->h = 24;
	tbSeed        = new TextBox(11, "");
	tbSeed->w     = 220;
	tbSeed->h     = 24;

	bTitle->msg = "World Name";
	bCreative->visible = false; bCreative->active = false;
	bSurvival->visible = false; bSurvival->active = false;
	bOldWorld->visible = false; bOldWorld->active = false;
	bInfiniteWorld->visible = false; bInfiniteWorld->active = false;

	buttons.push_back(bTitle);
	buttons.push_back(bBack);
	buttons.push_back(bNameContinue);
	buttons.push_back(bCreative);
	buttons.push_back(bSurvival);
	buttons.push_back(bOldWorld);
	buttons.push_back(bInfiniteWorld);
	textBoxes.push_back(tbWorldName);
	textBoxes.push_back(tbSeed);

	tabButtons.push_back(bBack);
	tabButtons.push_back(bNameContinue);
	tabButtons.push_back(bCreative);
	tabButtons.push_back(bSurvival);
	tabButtons.push_back(bOldWorld);
	tabButtons.push_back(bInfiniteWorld);

	tbWorldName->setFocus(minecraft);
}

void SimpleChooseLevelScreen::setupPositions()
{
	const int headerH = bTitle->height; // 26px
	const int bw = 100;
	const int bh = bBack->height; // 26px

	// Header bar: title spans between Back button and right edge
	bBack->x  = 0;
	bBack->y  = 0;
	bBack->width = bw;

	bTitle->x    = bBack->width;
	bTitle->y    = 0;
	bTitle->width = width - bBack->width;

	tbWorldName->w = width - 40;
	tbWorldName->x = 20;
	tbWorldName->y = headerH + (height - headerH) / 2 - 46;
	tbSeed->w = width - 40;
	tbSeed->x = 20;
	tbSeed->y = tbWorldName->y + tbWorldName->h + 22;
	bNameContinue->width = 120;
	bNameContinue->x = width / 2 - bNameContinue->width / 2;
	bNameContinue->y = tbSeed->y + tbSeed->h + 10;

	// Content buttons — two big buttons side by side in the center
	const int btnW  = (width / 2) - 16;
	const int btnH  = 36;
	const int btnY  = headerH + (height - headerH) / 2 - btnH / 2;

	bCreative->width  = btnW;  bCreative->height  = btnH;
	bSurvival->width  = btnW;  bSurvival->height  = btnH;
	bOldWorld->width  = btnW;  bOldWorld->height  = btnH;
	bInfiniteWorld->width  = btnW;  bInfiniteWorld->height  = btnH;

	bCreative->x  = 8;
	bCreative->y  = btnY;
	bSurvival->x  = width / 2 + 8;
	bSurvival->y  = btnY;

	bOldWorld->x      = 8;
	bOldWorld->y      = btnY;
	bInfiniteWorld->x = width / 2 + 8;
	bInfiniteWorld->y = btnY;
}

void SimpleChooseLevelScreen::render( int xm, int ym, float a )
{
	renderBackground();
    glEnable2(GL_BLEND);
	if (step == 0) {
		drawCenteredString(minecraft->font, "Enter world name", width / 2, tbWorldName->y - 14, 0xffaaaaaa);
		if (tbWorldName) tbWorldName->render(minecraft, xm, ym);
		drawCenteredString(minecraft->font, "Seed (optional)", width / 2, tbSeed->y - 12, 0xffaaaaaa);
		if (tbSeed) tbSeed->render(minecraft, xm, ym);
		Screen::render(xm, ym, a);
		glDisable2(GL_BLEND);
		return;
	}

	int descY = bCreative->y + bCreative->height + 6;
	if (step == 1) {
		drawCenteredString(minecraft->font, "Unlimited resources, no damage", bCreative->x + bCreative->width / 2, descY, 0xffaaaaaa);
		drawCenteredString(minecraft->font, "Survive, gather, build",          bSurvival->x + bSurvival->width / 2, descY, 0xffaaaaaa);
	} else {
		drawCenteredString(minecraft->font, "Finite 256x256",    bOldWorld->x      + bOldWorld->width      / 2, descY, 0xffaaaaaa);
		drawCenteredString(minecraft->font, "Endless world",     bInfiniteWorld->x + bInfiniteWorld->width / 2, descY, 0xffaaaaaa);
	}

	Screen::render(xm, ym, a);
    glDisable2(GL_BLEND);
}

void SimpleChooseLevelScreen::buttonClicked( Button* button )
{
	if (button == bBack) {
		if (step == 2) {
			step = 1;
			bTitle->msg = "Game Mode";
			bCreative->visible = true; bCreative->active = true;
			bSurvival->visible = true; bSurvival->active = true;
			bOldWorld->visible = false; bOldWorld->active = false;
			bInfiniteWorld->visible = false; bInfiniteWorld->active = false;
			bNameContinue->visible = false; bNameContinue->active = false;
			if (tbWorldName) tbWorldName->loseFocus(minecraft);
		} else if (step == 1) {
			step = 0;
			chosenGameType = -1;
			bTitle->msg = "World Name";
			bCreative->visible = false; bCreative->active = false;
			bSurvival->visible = false; bSurvival->active = false;
			bOldWorld->visible = false; bOldWorld->active = false;
			bInfiniteWorld->visible = false; bInfiniteWorld->active = false;
			bNameContinue->visible = true; bNameContinue->active = true;
			if (tbWorldName) tbWorldName->setFocus(minecraft);
		} else {
			minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
		}
		return;
	}

	if (step == 0) {
		if (button != bNameContinue)
			return;

		std::string candidate = Util::stringTrim(tbWorldName ? tbWorldName->text : std::string());
		if (candidate.empty())
			candidate = "world";
		levelName = candidate;
		seedText = tbSeed ? Util::stringTrim(tbSeed->text) : std::string();

		step = 1;
		bTitle->msg = "Game Mode";
		bNameContinue->visible = false; bNameContinue->active = false;
		bCreative->visible = true; bCreative->active = true;
		bSurvival->visible = true; bSurvival->active = true;
		if (tbWorldName) tbWorldName->loseFocus(minecraft);
		if (tbSeed) tbSeed->loseFocus(minecraft);
		return;
	}

	// Step 1: pick game mode
	if (step == 1) {
		if (button == bCreative)
			chosenGameType = GameType::Creative;
		else if (button == bSurvival)
			chosenGameType = GameType::Survival;
		else
			return;

		step = 2;
		bTitle->msg     = "World Type";
		bCreative->visible = false;  bCreative->active = false;
		bSurvival->visible = false;  bSurvival->active = false;
		bOldWorld->visible = true;   bOldWorld->active = true;
		bInfiniteWorld->visible = true;  bInfiniteWorld->active = true;
		return;
	}

	// Step 2: pick world type
	if (hasChosen) return;

	int worldType = -1;
	if (button == bOldWorld)
		worldType = WorldType::Old;
	else if (button == bInfiniteWorld)
		worldType = WorldType::Infinite;
	else
		return;

	std::string displayName = Util::stringTrim(levelName);
	if (displayName.empty()) displayName = "world";
	std::string levelId = displayName;
	for (int i = 0; i < sizeof(ILLEGAL_FILE_CHARACTERS) / sizeof(char); ++i)
		levelId = Util::stringReplace(levelId, std::string(1, ILLEGAL_FILE_CHARACTERS[i]), "");
	if (levelId.empty()) levelId = "world";
	levelId = getUniqueLevelName(levelId);
	int seed = getEpochTimeS();
	if (!seedText.empty()) {
		int tmpSeed;
		if (sscanf(seedText.c_str(), "%d", &tmpSeed) > 0)
			seed = tmpSeed;
		else
			seed = Util::hashCode(seedText);
	}
	LevelSettings settings(seed, chosenGameType, worldType);
	minecraft->selectLevel(levelId, displayName, settings);
	minecraft->hostMultiplayer();
	minecraft->setScreen(new ProgressScreen());
	hasChosen = true;
}

void SimpleChooseLevelScreen::keyboardNewChar(char inputChar)
{
	if (step != 0) return;

	if (tbWorldName && tbWorldName->focused) {
		if (inputChar >= 32 && inputChar < 127)
			tbWorldName->insertChar(inputChar, 64);
	} else if (tbSeed && tbSeed->focused) {
		if (inputChar >= 32 && inputChar < 127)
			tbSeed->insertChar(inputChar, 32);
	}
}

void SimpleChooseLevelScreen::keyPressed(int key)
{
	if (step == 0) {
		bool wFocus = tbWorldName && tbWorldName->focused;
		bool sFocus = tbSeed && tbSeed->focused;

		if (key == 9) { // Tab — switch focus between boxes
			if (wFocus) {
				tbWorldName->loseFocus(minecraft);
				if (tbSeed) tbSeed->setFocus(minecraft);
			} else {
				if (tbSeed) tbSeed->loseFocus(minecraft);
				if (tbWorldName) tbWorldName->setFocus(minecraft);
			}
			return;
		}
		if (wFocus && tbWorldName->handleEditKey(key)) return;
		if (sFocus && tbSeed->handleEditKey(key)) return;
		if (key == Keyboard::KEY_RETURN) {
			buttonClicked(bNameContinue);
			return;
		}
	}

	Screen::keyPressed(key);
}

void SimpleChooseLevelScreen::mouseClicked(int mx, int my, int btn)
{
	if (step == 0 && btn == MouseAction::ACTION_LEFT) {
		auto inBox = [](TextBox* tb, int x, int y) {
			return tb && x >= tb->x && x < tb->x + tb->w && y >= tb->y && y < tb->y + tb->h;
		};
		if (inBox(tbWorldName, mx, my)) {
			if (tbSeed) tbSeed->loseFocus(minecraft);
			tbWorldName->setFocus(minecraft);
		} else if (inBox(tbSeed, mx, my)) {
			if (tbWorldName) tbWorldName->loseFocus(minecraft);
			tbSeed->setFocus(minecraft);
		}
	}
	Screen::mouseClicked(mx, my, btn);
}

bool SimpleChooseLevelScreen::handleBackEvent(bool isDown) {
	if (!isDown) {
		if (step == 2) {
			step = 1;
			bTitle->msg = "Game Mode";
			bCreative->visible = true; bCreative->active = true;
			bSurvival->visible = true; bSurvival->active = true;
			bOldWorld->visible = false; bOldWorld->active = false;
			bInfiniteWorld->visible = false; bInfiniteWorld->active = false;
		} else if (step == 1) {
			step = 0;
			chosenGameType = -1;
			bTitle->msg = "World Name";
			bCreative->visible = false; bCreative->active = false;
			bSurvival->visible = false; bSurvival->active = false;
			bOldWorld->visible = false; bOldWorld->active = false;
			bInfiniteWorld->visible = false; bInfiniteWorld->active = false;
			bNameContinue->visible = true; bNameContinue->active = true;
			if (tbWorldName) tbWorldName->setFocus(minecraft);
		} else {
			minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
		}
	}
	return true;
}
