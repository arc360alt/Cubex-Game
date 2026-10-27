#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__OptionsScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__OptionsScreen_H__

#include "../Screen.h"
#include "../components/Button.h"

class ImageButton;
class OptionButton;
class OptionsPane;

class OptionsScreen: public Screen
{
	typedef Screen super;
	void init();

	void generateOptionScreens();
public:
	OptionsScreen();
	~OptionsScreen();
	void setupPositions();
	void buttonClicked( Button* button );
	void render(int xm, int ym, float a);
	void removed();
	void selectCategory(int index);
	void keyPressed(int key);

	virtual void mouseClicked( int x, int y, int buttonNum );
	virtual void mouseReleased( int x, int y, int buttonNum );
	virtual void keyboardNewChar(char inputChar);
	virtual void tick();
private:
	Touch::THeader* bHeader;
	ImageButton* btnClose;
	std::vector<Touch::TButton*> categoryButtons;
	std::vector<OptionsPane*> optionPanes;
	OptionsPane* currentOptionPane;
	Button* btnBrowseSkin;
	OptionButton* btnAlexSkin;
	TextBox* txtUsername;
	Button* btnLogin;
	Button* btnRegister;
	Button* btnLogout;
	Button* btnUploadSkin;
	Button* btnChangeUsername;
	TextBox* txtPassword;
	TextBox* txtNewUsername;
	std::string _accountStatus;
	int selectedCategory;

	// Cape picker (account tab, under Logout)
	std::vector<std::string> _capeChoices;       // admin-provided cape names from the backend
	std::vector<std::string> _capeTexturePaths;  // parallel array: local cached texture path per choice
	bool _capesFetched;
	void getCapeBoxRect(int index, int cx, int hH, int& bx, int& by, int& bw, int& bh) const;
	void applyCapeSelection(const std::string& capeName);

	// Account tab scroll state (its widgets are laid out below the header
	// and can run past the bottom of the screen)
	float _accountScrollY;
	bool _accountDragging;
	bool _accountDragMoved;
	int _accountDragStartY;
	float _accountDragStartScroll;
	int getAccountStatusY(int cx, int hH) const;
	int clampAccountScroll(); // returns the max scroll

	// Sidebar scroll state
	float _catScrollY;
	float _catScrollVelocity;
	int _catContentHeight;
	int _catVisibleHeight;
	bool _catDragging;
	int _catDragStartY;
	float _catDragStartScroll;
	int _catLastDragY;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__OptionsScreen_H__*/
