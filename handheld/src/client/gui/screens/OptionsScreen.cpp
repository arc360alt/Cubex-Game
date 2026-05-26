#include "OptionsScreen.h"

#include "StartMenuScreen.h"
#include "DialogDefinitions.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
#include "../../../locale/I18n.h"
#include <unistd.h>
#include "../../player/LocalPlayer.h"
#include "../../sound/SoundEngine.h"
#include "../../renderer/Textures.h"

#include "../components/OptionsPane.h"
#include "../components/ImageButton.h"
#include "../components/OptionsGroup.h"
#include "../Gui.h"
#include "../../renderer/gles.h"
#include "../../../platform/input/Mouse.h"
#include "../../../util/Mth.h"
#include <png.h>
#include "../components/TextBox.h"

static bool loadPNG(const std::string& path, TextureData& out) {
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) return false;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) { fclose(fp); return false; }

    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_read_struct(&png, NULL, NULL); fclose(fp); return false; }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    out.w = png_get_image_width(png, info);
    out.h = png_get_image_height(png, info);
    png_set_expand(png);
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    png_set_strip_16(png);
    png_read_update_info(png, info);

    out.numBytes = out.w * out.h * 4;
    out.data = new unsigned char[out.numBytes];
    out.format = TEXF_UNCOMPRESSED_8888;
    out.transparent = true;

    std::vector<png_bytep> rows(out.h);
    for (int i = 0; i < out.h; ++i)
        rows[i] = out.data + i * out.w * 4;

    png_read_image(png, rows.data());
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    return true;
}
OptionsScreen::OptionsScreen()
: btnClose(NULL),
  bHeader(NULL),
  btnBrowseSkin(NULL),
  btnAlexSkin(NULL),
  currentOptionPane(NULL),
  selectedCategory(0),
  _catScrollY(0),
  _catScrollVelocity(0),
  _catContentHeight(0),
  _catVisibleHeight(0),
  _catDragging(false),
  _catDragStartY(0),
  _catDragStartScroll(0),
  _catLastDragY(0) {
}

OptionsScreen::~OptionsScreen() {
	if(btnClose != NULL) {
		delete btnClose;
		btnClose = NULL;
	}
	if(bHeader != NULL) {
		delete bHeader,
		bHeader = NULL;
	}
	for(std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		if(*it != NULL) {
			delete *it;
			*it = NULL;
		}
	}
	for(std::vector<OptionsPane*>::iterator it = optionPanes.begin(); it != optionPanes.end(); ++it) {
		if(*it != NULL) {
			delete *it;
			*it = NULL;
		}
	}
	categoryButtons.clear();
	optionPanes.clear();
	currentOptionPane = NULL;
	if (btnBrowseSkin) { delete btnBrowseSkin; btnBrowseSkin = NULL; }
	if (btnAlexSkin) { delete btnAlexSkin; btnAlexSkin = NULL; }
}

void OptionsScreen::init() {
	bHeader = new Touch::THeader(0, "Options");
	btnClose = new ImageButton(1, "");
	ImageDef def;
	def.name = "gui/touchgui.png";
	def.width = 34;
	def.height = 26;

	btnBrowseSkin = new Button(10, "Browse Skin...");
	btnBrowseSkin->width = 120;
	btnBrowseSkin->height = 24;
	btnBrowseSkin->visible = false;

	{
		ImageDef alexDef;
		alexDef.setSrc(IntRectangle(160, 206, 39, 20));
		alexDef.name = "gui/touchgui.png";
		alexDef.width = 39 * 0.7f;
		alexDef.height = 20 * 0.7f;
		btnAlexSkin = new OptionButton(&Options::Option::ALEX_SKIN);
		btnAlexSkin->setImageDef(alexDef, true);
		btnAlexSkin->updateImage(&minecraft->options);
		btnAlexSkin->visible = false;
	} 

	txtUsername = new TextBox(11, "");
	txtUsername->text = minecraft->options.username;
	txtUsername->w = 120;
	txtUsername->h = 24;
	textBoxes.push_back(txtUsername);

	def.setSrc(IntRectangle(150, 0, (int)def.width, (int)def.height));
	btnClose->setImageDef(def, true);

	// Wider category buttons for desktop readability
	Touch::TButton* b;
	b = new Touch::TButton(2, I18n::get("options.category.audio"));    b->width = 80; b->height = 28; categoryButtons.push_back(b);
	b = new Touch::TButton(3, I18n::get("options.category.game"));     b->width = 80; b->height = 28; categoryButtons.push_back(b);
	b = new Touch::TButton(4, I18n::get("options.category.controls")); b->width = 80; b->height = 28; categoryButtons.push_back(b);
	b = new Touch::TButton(5, I18n::get("options.category.graphics")); b->width = 80; b->height = 28; categoryButtons.push_back(b);
	b = new Touch::TButton(6, "Skin");  b->width = 80; b->height = 28; categoryButtons.push_back(b);
	buttons.push_back(bHeader);
	buttons.push_back(btnClose);
	for(std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		buttons.push_back(*it);
		tabButtons.push_back(*it);
	}
	generateOptionScreens();

}
void OptionsScreen::keyboardNewChar(char inputChar) {
    if (txtUsername && txtUsername->focused) {
        if (inputChar >= 32 && inputChar < 127 && (int)txtUsername->text.size() < 14) {
            txtUsername->text += inputChar;
        }
    }
}
void OptionsScreen::setupPositions() {
	int headerH = btnClose->height;
	int catW = (categoryButtons.size() > 0) ? categoryButtons[0]->width : 80;

	btnClose->x = width - btnClose->width;
	btnClose->y = 0;

	bHeader->x = 0;
	bHeader->y = 0;
	bHeader->width = width - btnClose->width;
	bHeader->height = headerH;

	int curY = headerH + 2;
	for(std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		(*it)->x = 0;
		(*it)->y = curY;
		(*it)->selected = false;
		curY += (*it)->height + 1;
	}
	_catContentHeight = curY - (headerH + 2);

	for(std::vector<OptionsPane*>::iterator it = optionPanes.begin(); it != optionPanes.end(); ++it) {
		(*it)->x = catW + 2;
		(*it)->y = headerH + 2;
		(*it)->width = width - catW - 2;
		(*it)->setupPositions();
	}
	selectCategory(0);

	if (btnBrowseSkin) {
		int catW2 = (categoryButtons.size() > 0) ? categoryButtons[0]->width : 80;
		int headerH2 = (bHeader != NULL) ? bHeader->height : 26;
		btnBrowseSkin->x = catW2 + 6;
		btnBrowseSkin->y = headerH2 + 110;
	}
	if (btnAlexSkin) {
		int headerH2 = (bHeader != NULL) ? bHeader->height : 26;
		btnAlexSkin->x = width - 36;
		btnAlexSkin->y = headerH2 + 140;
	}
	if (txtUsername) {
		int catW2 = (categoryButtons.size() > 0) ? categoryButtons[0]->width : 80;
		int headerH2 = (bHeader != NULL) ? bHeader->height : 26;
		txtUsername->x = catW2 + 6;
		txtUsername->y = headerH2 + 170;
	}
}

void OptionsScreen::render( int xm, int ym, float a ) {
	renderBackground();

	int catW = (categoryButtons.size() > 0) ? categoryButtons[0]->width : 80;
	int headerH = (bHeader != NULL) ? bHeader->height : 26;

	// Sidebar background & border
	fill(0, headerH, catW, height, 0x66000000);
	fill(catW, headerH, catW + 1, height, 0x88888888);

	// Render header and close button (unscissored)
	if (bHeader != NULL) ((Button*)bHeader)->render(minecraft, xm, ym);
	if (btnClose != NULL) ((Button*)btnClose)->render(minecraft, xm, ym);

	// --- Scrollable sidebar category buttons ---
	int sidebarTop = headerH + 2;
	_catVisibleHeight = height - sidebarTop;
	if (_catVisibleHeight < 1) _catVisibleHeight = 1;
	int maxScroll = _catContentHeight - _catVisibleHeight;
	if (maxScroll < 0) maxScroll = 0;
	if (_catScrollY < 0) _catScrollY = 0;
	if (_catScrollY > maxScroll) _catScrollY = (float)maxScroll;

	float scale = Gui::GuiScale;
	int physH = minecraft->height;
	GLint sx = 0;
	GLint sy = physH - (GLint)(scale * (sidebarTop + _catVisibleHeight));
	GLsizei sw = (GLsizei)(scale * catW);
	GLsizei sh = (GLsizei)(scale * _catVisibleHeight);
	glEnable2(GL_SCISSOR_TEST);
	glScissor(sx, sy, sw, sh);

	glPushMatrix2();
	glTranslatef2(0, -(int)_catScrollY, 0);

	// Render only category buttons inside scissor
	for (size_t i = 0; i < categoryButtons.size(); ++i) {
		categoryButtons[i]->render(minecraft, xm, ym);
	}

	glPopMatrix2();

	// Sidebar scrollbar
	if (maxScroll > 0) {
		float barH = (float)_catVisibleHeight * _catVisibleHeight / _catContentHeight;
		if (barH < 8) barH = 8;
		float barY = sidebarTop + (_catScrollY / maxScroll) * (_catVisibleHeight - barH);
		int barX = catW - 3;
		fill(barX, (int)barY, barX + 2, (int)(barY + barH), 0x88ffffff);
	}

	glDisable2(GL_SCISSOR_TEST);

	// Content pane (outside scissor)
	int xmm = xm * width / minecraft->width;
	int ymm = ym * height / minecraft->height - 1;
	if(currentOptionPane != NULL)
		currentOptionPane->render(minecraft, xmm, ymm);
	if (selectedCategory == 4) {
		int catW = (categoryButtons.size() > 0) ? categoryButtons[0]->width : 80;
		int headerH = (bHeader != NULL) ? bHeader->height : 26;
		std::string homePath = std::string(getenv("HOME")) + "/.minecraft_skin.png";
		std::string current = minecraft->options.skinPath.empty() ? "Default (char.png)" : minecraft->options.skinPath;
		minecraft->font->drawShadow("Current skin:", (float)(catW + 6), (float)(headerH + 30), 0xffffffff);
		minecraft->font->drawShadow(current, (float)(catW + 6), (float)(headerH + 42), 0xffaaaaaa);
		minecraft->font->drawShadow("Place skin PNG at:", (float)(catW + 6), (float)(headerH + 62), 0xffffffff);
		minecraft->font->drawShadow(homePath, (float)(catW + 6), (float)(headerH + 74), 0xffaaaaaa);
		minecraft->font->drawShadow("[Press R to reload skin]", (float)(catW + 6), (float)(headerH + 94), 0xffcccccc);
	}
	if (selectedCategory == 4 && btnBrowseSkin && btnBrowseSkin->visible)
    	btnBrowseSkin->render(minecraft, xm, ym);

	if (selectedCategory == 4 && btnAlexSkin && btnAlexSkin->visible) {
		btnAlexSkin->render(minecraft, xm, ym);
		minecraft->font->drawShadow(I18n::get("options.alexSkin"), (float)(catW + 6), (float)(btnAlexSkin->y + 6), 0xe0e0e0);
	}

	if (selectedCategory == 4 && txtUsername) {
    minecraft->font->drawShadow("Username:", (float)txtUsername->x, (float)(txtUsername->y - 10), 0xffffffff);
    txtUsername->render(minecraft, xm, ym);
	}
}

void OptionsScreen::removed()
{
}
void OptionsScreen::buttonClicked(Button* button) {
    printf("buttonClicked: button=%p btnBrowseSkin=%p\n", button, btnBrowseSkin);
    fflush(stdout);
    if (button == btnClose) {
        minecraft->reloadOptions();
        minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
    } else if (button->id > 1 && button->id < 8) {
        int categoryButton = button->id - categoryButtons[0]->id;
        selectCategory(categoryButton);
    }
	if (button == btnBrowseSkin) {
		FILE* f = popen("zenity --file-selection --file-filter='PNG files | *.png' 2>/dev/null", "r");
		if (f) {
			char path[512] = {0};
			if (fgets(path, sizeof(path), f)) {
				int len = strlen(path);
				if (len > 0 && path[len-1] == '\n') path[len-1] = 0;
					if (strlen(path) > 0) {
						std::string skinPath = std::string(path);
						
						minecraft->options.skinPath = skinPath;
						minecraft->options.save();

						if (minecraft->player) {
							// Remove old skin from cache so it gets re-fetched
							minecraft->textures->reloadTexture(minecraft->player->getSkinTexture());
							
							// Point player to new skin file
							minecraft->player->setSkinTexture(skinPath);
							
							// Pre-load the new texture now
							TextureData td = minecraft->platform()->loadTexture(skinPath, false);
							if (td.data != NULL) {
								minecraft->textures->assignTexture(skinPath, td);
								delete[] td.data;
							}
						}
					}
			}
			pclose(f);
		}
	}
}

void OptionsScreen::selectCategory( int index ) {
    selectedCategory = index;
    int currentIndex = 0;
    for(std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
        if(index == currentIndex) {
            (*it)->selected = true;
        } else {
            (*it)->selected = false;
        }
        currentIndex++;
    }
    if(index < (int)optionPanes.size())
        currentOptionPane = optionPanes[index];

    if (btnBrowseSkin)
        btnBrowseSkin->visible = (selectedCategory == 4);
    if (btnAlexSkin)
        btnAlexSkin->visible = (selectedCategory == 4);
}

void OptionsScreen::keyPressed(int key) {
    fflush(stdout);
    if (txtUsername && txtUsername->focused) {
		if (key == Keyboard::KEY_BACKSPACE && !txtUsername->text.empty()) {
			txtUsername->text.pop_back();
			return;
		}
    }
    if (key == Keyboard::KEY_R && selectedCategory == 4) {
        std::string homePath = std::string(getenv("HOME")) + "/.minecraft_skin.png";
        if (access(homePath.c_str(), F_OK) == 0) {
            minecraft->options.skinPath = homePath;
        } else {
            minecraft->options.skinPath = "";
        }
        minecraft->options.save();
        if (minecraft->player) {
            std::string newSkin = minecraft->options.skinPath.empty() ? "mob/char.png" : minecraft->options.skinPath;
            ((LocalPlayer*)minecraft->player)->setSkinTexture(newSkin);
        }
    }
    super::keyPressed(key);
}

void OptionsScreen::generateOptionScreens() {
	optionPanes.push_back(new OptionsPane());
	optionPanes.push_back(new OptionsPane());
	optionPanes.push_back(new OptionsPane());
	optionPanes.push_back(new OptionsPane());
	optionPanes.push_back(new OptionsPane());
	// Audio
	optionPanes[0]->createOptionsGroup("options.group.audio")
		.addOptionItem(&Options::Option::MUSIC, minecraft)
		.addOptionItem(&Options::Option::SOUND, minecraft);

	// Game
	optionPanes[1]->createOptionsGroup("options.group.game")
		.addOptionItem(&Options::Option::DIFFICULTY, minecraft)
		.addOptionItem(&Options::Option::THIRD_PERSON, minecraft)
		.addOptionItem(&Options::Option::HIDE_GUI, minecraft)
		.addOptionItem(&Options::Option::SERVER_VISIBLE, minecraft);

	// Controls
	optionPanes[2]->createOptionsGroup("options.group.controls")
		.addOptionItem(&Options::Option::SENSITIVITY, minecraft)
		.addOptionItem(&Options::Option::INVERT_MOUSE, minecraft)
		.addOptionItem(&Options::Option::AUTO_JUMP, minecraft)
		.addOptionItem(&Options::Option::LEFT_HANDED, minecraft)
		.addOptionItem(&Options::Option::USE_TOUCH_JOYPAD, minecraft)
		.addOptionItem(&Options::Option::DESTROY_VIBRATION, minecraft);

	// Graphics
	optionPanes[3]->createOptionsGroup("options.group.graphics")
		.addOptionItem(&Options::Option::GRAPHICS, minecraft)
		.addOptionItem(&Options::Option::AMBIENT_OCCLUSION, minecraft)
		.addOptionItem(&Options::Option::VSYNC, minecraft)
		.addOptionItem(&Options::Option::MAX_FPS, minecraft)
		.addOptionItem(&Options::Option::RENDER_DISTANCE, minecraft)
		.addOptionItem(&Options::Option::GUI_SCALE, minecraft)
		.addOptionItem(&Options::Option::VIEW_BOBBING, minecraft)
		.addOptionItem(&Options::Option::FOV, minecraft);
	
	// Skin options are rendered directly in render(), not via the pane
// 	int mojangGroup = optionPanes[0]->createOptionsGroup("Mojang");
// 	static const int arr[] = {5,4,3,15};
// 	std::vector<int> vec (arr, arr + sizeof(arr) / sizeof(arr[0]) );
// 	optionPanes[0]->createStepSlider(minecraft, mojangGroup, "This works?", &Options::Option::DIFFICULTY, vec);
// 
// 	// Game Pane
// 	int gameGroup = optionPanes[1]->createOptionsGroup("Game");
// 	optionPanes[1]->createToggle(gameGroup, "Third person camera", &Options::Option::THIRD_PERSON);
// 	optionPanes[1]->createToggle(gameGroup, "Server visible", &Options::Option::SERVER_VISIBLE);
// 	
// 	// Input Pane
// 	int controlsGroup = optionPanes[2]->createOptionsGroup("Controls");
// 	optionPanes[2]->createToggle(controlsGroup, "Invert X-axis", &Options::Option::INVERT_MOUSE);
// 	optionPanes[2]->createToggle(controlsGroup, "Lefty", &Options::Option::LEFT_HANDED);
// 	optionPanes[2]->createToggle(controlsGroup, "Use touch screen", &Options::Option::USE_TOUCHSCREEN);
// 	optionPanes[2]->createToggle(controlsGroup, "Split touch controls", &Options::Option::USE_TOUCH_JOYPAD);
// 	int feedBackGroup = optionPanes[2]->createOptionsGroup("Feedback");
// 	optionPanes[2]->createToggle(feedBackGroup, "Vibrate on destroy", &Options::Option::DESTROY_VIBRATION);
// 
// 	int graphicsGroup = optionPanes[3]->createOptionsGroup("Graphics");
// 	optionPanes[3]->createProgressSlider(minecraft, graphicsGroup, "Gui Scale", &Options::Option::PIXELS_PER_MILLIMETER, 3, 4);
// 	optionPanes[3]->createToggle(graphicsGroup, "Fancy Graphics", &Options::Option::INVERT_MOUSE);
// 	optionPanes[3]->createToggle(graphicsGroup, "Fancy Skies", &Options::Option::INVERT_MOUSE);
// 	optionPanes[3]->createToggle(graphicsGroup, "Animated water", &Options::Option::INVERT_MOUSE);
// 	int experimentalGraphicsGroup = optionPanes[3]->createOptionsGroup("Experimental graphics");
// 	optionPanes[3]->createToggle(experimentalGraphicsGroup, "Soft shadows", &Options::Option::INVERT_MOUSE);
}

void OptionsScreen::mouseClicked( int x, int y, int buttonNum ) {
	int catW = (categoryButtons.size() > 0) ? categoryButtons[0]->width : 80;
	int headerH = (bHeader != NULL) ? bHeader->height : 26;
	int sidebarTop = headerH + 2;

	if (x >= 0 && x < catW && y >= sidebarTop) {
		// Click in sidebar — start drag
		_catDragging = true;
		_catDragStartY = y;
		_catDragStartScroll = _catScrollY;
		_catLastDragY = y;
		_catScrollVelocity = 0;
		// Pass click with scroll offset to buttons
		super::mouseClicked(x, y + (int)_catScrollY, buttonNum);
	} else {
		if(currentOptionPane != NULL)
			currentOptionPane->mouseClicked(minecraft, x, y, buttonNum);
		super::mouseClicked(x, y, buttonNum);
	}
	if (txtUsername && selectedCategory == 4)
    txtUsername->setFocus(minecraft);
}

void OptionsScreen::mouseReleased(int x, int y, int buttonNum) {
    int catW = (categoryButtons.size() > 0) ? categoryButtons[0]->width : 80;
    int headerH = (bHeader != NULL) ? bHeader->height : 26;
    int sidebarTop = headerH + 2;

    _catDragging = false;

    LOGI("mouseReleased: x=%d y=%d btn=%d | browseSkin visible=%d bounds=(%d,%d,%d,%d)\n",
        x, y, buttonNum,
        btnBrowseSkin ? btnBrowseSkin->visible : -1,
        btnBrowseSkin ? btnBrowseSkin->x : -1,
        btnBrowseSkin ? btnBrowseSkin->y : -1,
        btnBrowseSkin ? btnBrowseSkin->x + btnBrowseSkin->width : -1,
        btnBrowseSkin ? btnBrowseSkin->y + btnBrowseSkin->height : -1);

    // Manual hit-test for btnBrowseSkin
    if (btnBrowseSkin && btnBrowseSkin->visible && buttonNum == 1) {
        if (x >= btnBrowseSkin->x && x < btnBrowseSkin->x + btnBrowseSkin->width &&
            y >= btnBrowseSkin->y && y < btnBrowseSkin->y + btnBrowseSkin->height) {
            buttonClicked(btnBrowseSkin);
            clickedButton = NULL;
            return;
        }
    }

    // Manual hit-test for btnAlexSkin
    if (btnAlexSkin && btnAlexSkin->visible && buttonNum == 1) {
        if (x >= btnAlexSkin->x && x < btnAlexSkin->x + btnAlexSkin->width &&
            y >= btnAlexSkin->y && y < btnAlexSkin->y + btnAlexSkin->height) {
            btnAlexSkin->toggle(&minecraft->options);
            clickedButton = NULL;
            return;
        }
    }

    if (x >= 0 && x < catW && y >= sidebarTop) {
        int adjustedY = y + (int)_catScrollY;
        for (size_t i = 0; i < categoryButtons.size(); ++i) {
            if (clickedButton == categoryButtons[i] &&
                categoryButtons[i]->clicked(minecraft, x, adjustedY)) {
                buttonClicked(categoryButtons[i]);
                minecraft->soundEngine->playUI("random.click", 1, 1);
                categoryButtons[i]->released(x, adjustedY);
            }
        }
        clickedButton = NULL;
    } else {
        if (currentOptionPane != NULL)
            currentOptionPane->mouseReleased(minecraft, x, y, buttonNum);
        super::mouseReleased(x, y, buttonNum);
    }
}

void OptionsScreen::tick() {
	// Sidebar scroll handling
	if (_catDragging) {
		int mx = Mouse::getX();
		int my = Mouse::getY();
		toGUICoordinate(mx, my);
		float delta = (float)(_catDragStartY - my);
		_catScrollVelocity = (float)(_catLastDragY - my);
		_catLastDragY = my;
		_catScrollY = _catDragStartScroll + delta;
	} else if (_catScrollVelocity > 0.5f || _catScrollVelocity < -0.5f) {
		_catScrollY += _catScrollVelocity;
		_catScrollVelocity *= 0.85f;
	}

	if(currentOptionPane != NULL)
		currentOptionPane->tick(minecraft);
	super::tick();
	if (txtUsername && selectedCategory == 4) {
		if (txtUsername->text != minecraft->options.username) {
			minecraft->options.username = txtUsername->text;
			minecraft->options.save();
			if (minecraft->user)
				minecraft->user->name = txtUsername->text;
		}
	}
}
