#include "OptionsScreen.h"

#include "StartMenuScreen.h"
#include "DialogDefinitions.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
#include "../../../locale/I18n.h"
#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#define F_OK 0
#define access _access
#define popen _popen
#define pclose _pclose
#endif
#include "../../player/LocalPlayer.h"
#include "../../sound/SoundEngine.h"
#include "../../renderer/Textures.h"
#include "../../renderer/Tesselator.h"
#include "../../BackendClient.h"

#include "../components/OptionsPane.h"
#include "../components/ImageButton.h"
#include "../components/OptionsGroup.h"
#include "../Gui.h"
#include "../../renderer/gles.h"
#include "../../../platform/input/Mouse.h"
#include "../../../util/Mth.h"
#ifndef __ANDROID__
#include <png.h>
#endif
#include "../components/TextBox.h"

#ifndef __ANDROID__
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
#endif
// Draws one UV region of a skin texture as a scaled quad
static void drawSkinRegion(Textures* textures, TextureId texId,
                           float dx, float dy, float dw, float dh,
                           float u0, float v0, float u1, float v1) {
    textures->bind(texId);
    glEnable2(GL_TEXTURE_2D);
    glEnable2(GL_BLEND);
    glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Tesselator& t = Tesselator::instance;
    t.begin();
    t.color(1.0f, 1.0f, 1.0f, 1.0f); // reset tint to white
    t.vertexUV(dx,      dy + dh, 0, u0, v1);
    t.vertexUV(dx + dw, dy + dh, 0, u1, v1);
    t.vertexUV(dx + dw, dy,      0, u1, v0);
    t.vertexUV(dx,      dy,      0, u0, v0);
    t.draw();
    glDisable2(GL_BLEND);
}

// Draws a front-facing player head (base layer + hat overlay) from a skin texture
static void drawSkinPreview(Minecraft* mc, const std::string& skinPath, float x, float y, float size) {
    std::string path = skinPath.empty() ? "mob/char.png" : skinPath;
    TextureId texId = mc->textures->loadTexture(path, skinPath.empty());
    if (!Textures::isTextureIdValid(texId)) return;

    // Detect actual texture height to handle both 64x32 and 64x64 skins
    float tw = 64.0f, th = 32.0f;
    const TextureData* td = mc->textures->getTemporaryTextureData(texId);
    if (td) { tw = (float)td->w; th = (float)td->h; }

    // Head front face: pixel (8,8)-(16,16)
    float u0 = 8/tw, u1 = 16/tw, v0 = 8/th, v1 = 16/th;
    drawSkinRegion(mc->textures, texId, x, y, size, size, u0, v0, u1, v1);

    // Hat overlay: pixel (40,8)-(48,16)
    float hu0 = 40/tw, hu1 = 48/tw;
    drawSkinRegion(mc->textures, texId, x, y, size, size, hu0, v0, hu1, v1);
}

OptionsScreen::OptionsScreen()
: btnClose(NULL),
  bHeader(NULL),
  btnBrowseSkin(NULL),
  btnAlexSkin(NULL),
  currentOptionPane(NULL),
  btnLogin(NULL),
  btnRegister(NULL),
  btnLogout(NULL),
  btnUploadSkin(NULL),
  btnChangeUsername(NULL),
  txtPassword(NULL),
  txtNewUsername(NULL),
  _accountStatus(""),
  selectedCategory(0),
  _capesFetched(false),
  _catScrollY(0),
  _catScrollVelocity(0),
  _catContentHeight(0),
  _catVisibleHeight(0),
  _catDragging(false),
  _catDragStartY(0),
  _catDragStartScroll(0),
  _catLastDragY(0),
  _accountScrollY(0),
  _accountDragging(false),
  _accountDragMoved(false),
  _accountDragStartY(0),
  _accountDragStartScroll(0) {
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
	if (btnLogin) { delete btnLogin; btnLogin = NULL; }
	if (btnRegister) { delete btnRegister; btnRegister = NULL; }
	if (btnLogout) { delete btnLogout; btnLogout = NULL; }
	if (btnUploadSkin) { delete btnUploadSkin; btnUploadSkin = NULL; }
	if (btnChangeUsername) { delete btnChangeUsername; btnChangeUsername = NULL; }
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

	txtPassword = new TextBox(12, "");
	txtPassword->password = true;
	txtPassword->text = "";
	txtPassword->w = 120;
	txtPassword->h = 24;
	textBoxes.push_back(txtPassword);

	txtNewUsername = new TextBox(13, "");
	txtNewUsername->text = "";
	txtNewUsername->w = 120;
	txtNewUsername->h = 24;
	textBoxes.push_back(txtNewUsername);

	btnLogin = new Button(12, "Login");
	btnLogin->width = 58; btnLogin->height = 22; btnLogin->visible = false;
	btnRegister = new Button(13, "Register");
	btnRegister->width = 58; btnRegister->height = 22; btnRegister->visible = false;
	btnLogout = new Button(14, "Logout");
	btnLogout->width = 80; btnLogout->height = 22; btnLogout->visible = false;
	btnUploadSkin = new Button(15, "Upload Skin");
	btnUploadSkin->width = 80; btnUploadSkin->height = 22; btnUploadSkin->visible = false;
	btnChangeUsername = new Button(16, "Change Username");
	btnChangeUsername->width = 120; btnChangeUsername->height = 22; btnChangeUsername->visible = false;

	def.setSrc(IntRectangle(150, 0, (int)def.width, (int)def.height));
	btnClose->setImageDef(def, true);

	// Wider category buttons for desktop readability
	Touch::TButton* b;
	b = new Touch::TButton(2, I18n::get("options.category.audio"));    b->width = 80; b->height = 28; categoryButtons.push_back(b);
	b = new Touch::TButton(3, I18n::get("options.category.game"));     b->width = 80; b->height = 28; categoryButtons.push_back(b);
	b = new Touch::TButton(4, I18n::get("options.category.controls")); b->width = 80; b->height = 28; categoryButtons.push_back(b);
	b = new Touch::TButton(5, I18n::get("options.category.graphics")); b->width = 80; b->height = 28; categoryButtons.push_back(b);
	b = new Touch::TButton(6, "Account"); b->width = 80; b->height = 28; categoryButtons.push_back(b);
	buttons.push_back(bHeader);
	buttons.push_back(btnClose);
	for(std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		buttons.push_back(*it);
		tabButtons.push_back(*it);
	}
	generateOptionScreens();

}
void OptionsScreen::keyboardNewChar(char inputChar) {
    if (inputChar < 32 || inputChar >= 127) return;
    if (txtUsername && txtUsername->focused)
        txtUsername->insertChar(inputChar, 25);
    if (txtPassword && txtPassword->focused)
        txtPassword->insertChar(inputChar, 64);
    if (txtNewUsername && txtNewUsername->focused)
        txtNewUsername->insertChar(inputChar, 25);
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
	{
		int cW = (categoryButtons.size() > 0) ? categoryButtons[0]->width : 80;
		int hH = (bHeader != NULL) ? bHeader->height : 26;
		int cx = cW + 6;
		if (txtUsername)      { txtUsername->x      = cx;      txtUsername->y      = hH + 54; }
		if (txtPassword)      { txtPassword->x      = cx;      txtPassword->y      = hH + 90; }
		if (btnLogin)         { btnLogin->x         = cx;      btnLogin->y         = hH + 120; }
		if (btnRegister)      { btnRegister->x      = cx + 62; btnRegister->y      = hH + 120; }
		if (txtNewUsername)   { txtNewUsername->x   = cx;      txtNewUsername->y   = hH + 72; }
		if (btnChangeUsername){ btnChangeUsername->x = cx;     btnChangeUsername->y = hH + 100; }
		if (btnUploadSkin)   { btnUploadSkin->x    = cx;      btnUploadSkin->y    = hH + 128; }
		if (btnLogout)       { btnLogout->x        = cx;      btnLogout->y        = hH + 156; }
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
	if (!_catDragging && xm >= 0 && xm < catW && ym >= sidebarTop) {
		float s = Mouse::consumeScrollY();
		if (s != 0) {
			_catScrollY -= s * Mouse::SCROLL_PIXELS_PER_UNIT;
			_catScrollVelocity = 0;
		}
	}
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

	// Account tab scrolling. Taken before the option pane renders, which
	// would otherwise grab the scroll input over the same area.
	int accountScroll = 0;
	int accountMaxScroll = 0;
	if (selectedCategory == 4) {
		if (!_accountDragging && xm > catW && ym >= headerH) {
			float s = Mouse::consumeScrollY();
			if (s != 0) _accountScrollY -= s * Mouse::SCROLL_PIXELS_PER_UNIT;
		}
		accountMaxScroll = clampAccountScroll();
		accountScroll = (int)_accountScrollY;
	}

	// Content pane (outside scissor)
	int xmm = xm * width / minecraft->width;
	int ymm = ym * height / minecraft->height - 1;
	if(currentOptionPane != NULL)
		currentOptionPane->render(minecraft, xmm, ymm);
	if (selectedCategory == 4) {
		int cW = (categoryButtons.size() > 0) ? categoryButtons[0]->width : 80;
		int hH = (bHeader != NULL) ? bHeader->height : 26;
		float cx = (float)(cW + 6);
		bool loggedIn = !minecraft->options.accountToken.empty();

		// Clip below the header and shift everything up by the scroll offset;
		// widgets keep their unscrolled positions, so the mouse is shifted too
		glEnable2(GL_SCISSOR_TEST);
		glScissor((GLint)(scale * (cW + 1)), 0, (GLsizei)(scale * (width - cW - 1)), (GLsizei)(scale * (height - hH)));
		glPushMatrix2();
		glTranslatef2(0, (float)-accountScroll, 0);
		ym += accountScroll;

		if (!loggedIn) {
			minecraft->font->drawShadow("Login or Register", cx, (float)(hH + 34), 0xffffffff);
			minecraft->font->drawShadow("Username:", cx, (float)(hH + 44), 0xffcccccc);
			if (txtUsername) txtUsername->render(minecraft, xm, ym);
			minecraft->font->drawShadow("Password:", cx, (float)(hH + 80), 0xffcccccc);
			if (txtPassword) txtPassword->render(minecraft, xm, ym);
			if (btnLogin) btnLogin->render(minecraft, xm, ym);
			if (btnRegister) btnRegister->render(minecraft, xm, ym);
		} else {
			std::string loggedAs = "Logged in as: " + minecraft->options.accountUsername;
			minecraft->font->drawShadow(loggedAs, cx, (float)(hH + 34), 0xff88ff88);
			std::string uuidLine = "UUID: " + (minecraft->options.accountUuid.empty() ? std::string("(fetching...)") : minecraft->options.accountUuid);
			minecraft->font->drawShadow(uuidLine, cx, (float)(hH + 46), 0xffaaaaaa);
			minecraft->font->drawShadow("New username:", cx, (float)(hH + 62), 0xffcccccc);
			if (txtNewUsername) txtNewUsername->render(minecraft, xm, ym);
			if (btnChangeUsername) btnChangeUsername->render(minecraft, xm, ym);
			if (btnUploadSkin) btnUploadSkin->render(minecraft, xm, ym);
			if (btnLogout) btnLogout->render(minecraft, xm, ym);

			// Skin preview — head face to the right of the buttons. Pushed down
			// below the "Logged in as" / "UUID" lines so a full-length uuid
			// string (which is wide enough to reach this column) doesn't run
			// underneath the preview box.
			float px = cx + 160, py = (float)(hH + 58);
			float psize = 64;
			fill((int)px - 3, (int)py - 3, (int)(px + psize + 3), (int)(py + psize + 3), 0x88000000);
			drawSkinPreview(minecraft, minecraft->options.skinPath, px, py, psize);
			minecraft->font->drawShadow("Skin Preview", px, py + psize + 4, 0xffaaaaaa);

			// Cape picker — a row of small preview boxes below Logout. "None"
			// is always first, then whatever admin-provided capes the backend
			// currently has (see backend/capes/).
			minecraft->font->drawShadow("Cape:", cx, (float)(hH + 188), 0xffcccccc);
			{
				int bx, by, bw, bh;
				getCapeBoxRect(-1, (int)cx, hH, bx, by, bw, bh);
				bool noneSelected = minecraft->options.capeName.empty();
				fill(bx - 1, by - 1, bx + bw + 1, by + bh + 1, noneSelected ? 0xffffffff : 0xff555555);
				fill(bx, by, bx + bw, by + bh, 0xff000000);
				minecraft->font->drawShadow("x", (float)(bx + bw / 2 - 2), (float)(by + bh / 2 - 4), 0xffaaaaaa);

				for (size_t i = 0; i < _capeChoices.size(); ++i) {
					getCapeBoxRect((int)i, (int)cx, hH, bx, by, bw, bh);
					bool selected = (minecraft->options.capeName == _capeChoices[i]);
					fill(bx - 1, by - 1, bx + bw + 1, by + bh + 1, selected ? 0xffffffff : 0xff555555);
					fill(bx, by, bx + bw, by + bh, 0xff000000);
					if (i < _capeTexturePaths.size() && !_capeTexturePaths[i].empty()) {
						TextureId texId = minecraft->textures->loadTexture(_capeTexturePaths[i], false);
						if (Textures::isTextureIdValid(texId)) {
							float tw = 64.0f, th = 32.0f;
							const TextureData* td = minecraft->textures->getTemporaryTextureData(texId);
							if (td) { tw = (float)td->w; th = (float)td->h; }
							// Cape "front" panel only — pixels (1,1)-(11,17) in
							// the standard vanilla cape UV layout.
							drawSkinRegion(minecraft->textures, texId, (float)bx, (float)by, (float)bw, (float)bh,
							               1.0f / tw, 1.0f / th, 11.0f / tw, 17.0f / th);
						}
					}
				}
			}
		}
		if (!_accountStatus.empty())
			minecraft->font->drawShadow(_accountStatus, cx, (float)getAccountStatusY((int)cx, hH), 0xffff8844);

		glPopMatrix2();
		ym -= accountScroll;

		if (accountMaxScroll > 0) {
			int visibleH = height - hH;
			float barH = (float)visibleH * visibleH / (visibleH + accountMaxScroll);
			if (barH < 8) barH = 8;
			float barY = hH + (_accountScrollY / accountMaxScroll) * (visibleH - barH);
			int barX = width - 3;
			fill(barX, (int)barY, barX + 2, (int)(barY + barH), 0x88ffffff);
		}
		glDisable2(GL_SCISSOR_TEST);
	}
}

// Where the account status line goes: below the cape picker, which can wrap
// onto several rows
int OptionsScreen::getAccountStatusY(int cx, int hH) const {
	int y = hH + 226;
	if (!minecraft->options.accountToken.empty()) {
		int bx, by, bw, bh;
		getCapeBoxRect((int)_capeChoices.size() - 1, cx, hH, bx, by, bw, bh);
		if (by + bh + 6 > y) y = by + bh + 6;
	}
	return y;
}

int OptionsScreen::clampAccountScroll() {
	int cW = (categoryButtons.size() > 0) ? categoryButtons[0]->width : 80;
	int hH = (bHeader != NULL) ? bHeader->height : 26;
	int contentBottom = getAccountStatusY(cW + 6, hH) + 14;
	int maxScroll = contentBottom - height;
	if (maxScroll < 0) maxScroll = 0;
	if (_accountScrollY < 0) _accountScrollY = 0;
	if (_accountScrollY > maxScroll) _accountScrollY = (float)maxScroll;
	return maxScroll;
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
	if (button == btnLogin || button == btnRegister) {
		std::string user = txtUsername ? txtUsername->text : "";
		std::string pass = txtPassword ? txtPassword->text : "";
		if (user.empty() || pass.empty()) { _accountStatus = "Enter username and password."; return; }
		std::string uuid;
		std::string token;
		if (button == btnLogin)
			token = BackendClient::login(minecraft->options.backendUrl, user, pass, uuid);
		else
			token = BackendClient::registerUser(minecraft->options.backendUrl, user, pass, uuid);
		if (!token.empty()) {
			minecraft->options.accountToken = token;
			minecraft->options.accountUsername = user;
			minecraft->options.accountUuid = uuid;
			minecraft->options.username = user;
			minecraft->options.save();
			if (minecraft->user) minecraft->user->name = user;
			if (txtPassword) txtPassword->text = "";
			_accountStatus = (button == btnLogin) ? "Logged in!" : "Registered!";
			selectCategory(4); // refresh visibility
		} else {
			_accountStatus = (button == btnLogin) ? "Login failed." : "Register failed (name taken?).";
		}
	}
	if (button == btnLogout) {
		minecraft->options.accountToken = "";
		minecraft->options.accountUsername = "";
		minecraft->options.accountUuid = "";
		minecraft->options.save();
		_accountStatus = "Logged out.";
		selectCategory(4);
	}
	if (button == btnChangeUsername) {
		std::string newName = txtNewUsername ? txtNewUsername->text : "";
		if (newName.empty()) { _accountStatus = "Enter a new username."; return; }
		std::string result = BackendClient::changeUsername(minecraft->options.backendUrl,
		                                                   minecraft->options.accountToken, newName);
		if (!result.empty()) {
			minecraft->options.accountUsername = result;
			minecraft->options.username = result;
			minecraft->options.save();
			if (minecraft->user) minecraft->user->name = result;
			if (txtNewUsername) txtNewUsername->text = "";
			_accountStatus = "Username changed to: " + result;
		} else {
			_accountStatus = "Change failed (name taken?).";
		}
	}
	if (button == btnUploadSkin) {
		FILE* f = popen("zenity --file-selection --file-filter='PNG files | *.png' 2>/dev/null", "r");
		if (!f) { _accountStatus = "Could not open file picker."; return; }
		char path[512] = {0};
		bool picked = false;
		if (fgets(path, sizeof(path), f)) {
			int len = strlen(path);
			if (len > 0 && path[len-1] == '\n') path[len-1] = 0;
			picked = strlen(path) > 0;
		}
		pclose(f);
		if (!picked) return;
		std::string skinPath(path);
		bool ok = BackendClient::uploadSkin(minecraft->options.backendUrl,
		                                    minecraft->options.accountToken, skinPath);
		if (ok) {
			minecraft->options.skinPath = skinPath;
			minecraft->options.save();
			if (minecraft->player) {
				TextureData td = minecraft->platform()->loadTexture(skinPath, false);
				if (td.data) {
					minecraft->textures->assignTexture(skinPath, td);
					delete[] td.data;
				}
				minecraft->player->setSkinTexture(skinPath);
			}
			_accountStatus = "Skin uploaded!";
		} else {
			_accountStatus = "Upload failed.";
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
        btnBrowseSkin->visible = false;
    if (btnAlexSkin)
        btnAlexSkin->visible = false;

    bool onAccount = (selectedCategory == 4);
    bool loggedIn = onAccount && minecraft && !minecraft->options.accountToken.empty();
    if (loggedIn && minecraft->options.accountUuid.empty()) {
        // Accounts that logged in before uuids existed won't have one cached
        // locally yet — contacting the server here (the same call used to
        // confirm the token is still valid) is enough to get one backfilled.
        std::string uuid;
        std::string verifiedName = BackendClient::verifyToken(minecraft->options.backendUrl, minecraft->options.accountToken, uuid);
        if (!verifiedName.empty() && !uuid.empty()) {
            minecraft->options.accountUuid = uuid;
            minecraft->options.save();
        }
    }
    if (loggedIn && !_capesFetched) {
        // Fetch the catalog once per login, and eagerly cache each cape's
        // texture now (rather than in render(), which runs every frame and
        // would otherwise block on the first, uncached download).
        _capesFetched = true;
        _capeChoices = BackendClient::listCapes(minecraft->options.backendUrl);
        _capeTexturePaths.clear();
        for (size_t i = 0; i < _capeChoices.size(); ++i) {
            _capeTexturePaths.push_back(BackendClient::resolveCapeTexturePath(minecraft->options.backendUrl, _capeChoices[i]));
        }

        // Also pull down this account's *actual* current selection from the
        // server and adopt it locally. options.capeName previously only ever
        // got set by clicking a cape box on this specific install, so it
        // never reflected a cape chosen from another device/session — the
        // picker would show "None" highlighted (and the player would have no
        // cape) even though the account really has one selected.
        std::string serverCape = BackendClient::getPlayerCape(minecraft->options.backendUrl, minecraft->options.accountUsername);
        if (serverCape != minecraft->options.capeName) {
            minecraft->options.capeName = serverCape;
            minecraft->options.save();
            if (minecraft->player) {
                std::string capePath = BackendClient::resolveCapeTexturePath(minecraft->options.backendUrl, serverCape);
                minecraft->player->setCapeTexture(capePath);
            }
        }
    }
    if (!loggedIn) {
        // Re-fetch next time they log in, in case the catalog changed.
        _capesFetched = false;
    }
    if (btnLogin)         btnLogin->visible         = onAccount && !loggedIn;
    if (btnRegister)      btnRegister->visible      = onAccount && !loggedIn;
    if (btnLogout)        btnLogout->visible        = loggedIn;
    if (btnUploadSkin)    btnUploadSkin->visible    = loggedIn;
    if (btnChangeUsername) btnChangeUsername->visible = loggedIn;
}

// index -1 = "None" box, 0..N-1 = _capeChoices[index]. Shared by render()
// and mouseReleased() so hit-testing always matches what's drawn.
void OptionsScreen::getCapeBoxRect(int index, int cx, int hH, int& bx, int& by, int& bw, int& bh) const {
    bw = bh = 20;
    const int gap = 4;
    // Wrap onto more rows when the capes don't fit across the screen
    int perRow = (width - cx - 4) / (bw + gap);
    if (perRow < 1) perRow = 1;
    int slot = index + 1;
    by = hH + 200 + (slot / perRow) * (bh + gap);
    bx = cx + (slot % perRow) * (bw + gap);
}

void OptionsScreen::applyCapeSelection(const std::string& capeName) {
    std::string result;
    bool ok = BackendClient::setCape(minecraft->options.backendUrl, minecraft->options.accountToken, capeName, result);
    if (!ok) {
        _accountStatus = "Failed to set cape.";
        return;
    }
    minecraft->options.capeName = result;
    minecraft->options.save();
    if (minecraft->player) {
        std::string capePath = BackendClient::resolveCapeTexturePath(minecraft->options.backendUrl, result);
        minecraft->player->setCapeTexture(capePath);
    }
    _accountStatus = result.empty() ? "Cape removed." : ("Cape set to: " + result);
}

void OptionsScreen::keyPressed(int key) {
    fflush(stdout);
    if (txtUsername && txtUsername->focused && txtUsername->handleEditKey(key)) return;
    if (txtPassword && txtPassword->focused && txtPassword->handleEditKey(key)) return;
    if (txtNewUsername && txtNewUsername->focused && txtNewUsername->handleEditKey(key)) return;
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

		if (selectedCategory == 4 && x >= catW && y >= headerH) {
			// Click in the account area — start a possible drag-scroll
			_accountDragging = true;
			_accountDragMoved = false;
			_accountDragStartY = y;
			_accountDragStartScroll = _accountScrollY;
		}
	}
	if (selectedCategory == 4 && y >= headerH) {
		y += (int)_accountScrollY; // widgets are laid out unscrolled
		// Clear all account textbox focus first, then set focus on the clicked one
		auto clearAccountFocus = [&]() {
			if (txtUsername)    txtUsername->focused    = false;
			if (txtPassword)    txtPassword->focused    = false;
			if (txtNewUsername) txtNewUsername->focused = false;
		};
		auto inBox = [](TextBox* b, int px, int py) {
			return b && px >= b->x && px < b->x + b->w && py >= b->y && py < b->y + b->h;
		};
		bool loggedIn = !minecraft->options.accountToken.empty();
		if (!loggedIn) {
			if (inBox(txtUsername, x, y))   { clearAccountFocus(); txtUsername->setFocus(minecraft); }
			else if (inBox(txtPassword, x, y)) { clearAccountFocus(); txtPassword->setFocus(minecraft); }
		} else {
			if (inBox(txtNewUsername, x, y)) { clearAccountFocus(); txtNewUsername->setFocus(minecraft); }
		}
	}
}

void OptionsScreen::mouseReleased(int x, int y, int buttonNum) {
    int catW = (categoryButtons.size() > 0) ? categoryButtons[0]->width : 80;
    int headerH = (bHeader != NULL) ? bHeader->height : 26;
    int sidebarTop = headerH + 2;

    _catDragging = false;

    // Releasing a drag-scroll of the account tab isn't a click
    bool accountDragged = _accountDragging && _accountDragMoved;
    _accountDragging = false;
    if (accountDragged) {
        clickedButton = NULL;
        return;
    }
    // Account widgets are laid out unscrolled
    const int ay = (selectedCategory == 4 && y >= headerH) ? y + (int)_accountScrollY : -1;

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

    auto accountBtnHit = [&](Button* btn) -> bool {
        return btn && btn->visible && buttonNum == 1 &&
               x >= btn->x && x < btn->x + btn->width &&
               ay >= btn->y && ay < btn->y + btn->height;
    };
    if (accountBtnHit(btnLogin))          { buttonClicked(btnLogin);         clickedButton = NULL; return; }
    if (accountBtnHit(btnRegister))       { buttonClicked(btnRegister);      clickedButton = NULL; return; }
    if (accountBtnHit(btnLogout))         { buttonClicked(btnLogout);        clickedButton = NULL; return; }
    if (accountBtnHit(btnUploadSkin))     { buttonClicked(btnUploadSkin);    clickedButton = NULL; return; }
    if (accountBtnHit(btnChangeUsername)) { buttonClicked(btnChangeUsername); clickedButton = NULL; return; }

    if (selectedCategory == 4 && buttonNum == 1 && !minecraft->options.accountToken.empty()) {
        int cW = (categoryButtons.size() > 0) ? categoryButtons[0]->width : 80;
        int hH = (bHeader != NULL) ? bHeader->height : 26;
        int cx = cW + 6;
        int bx, by, bw, bh;

        getCapeBoxRect(-1, cx, hH, bx, by, bw, bh);
        if (x >= bx && x < bx + bw && ay >= by && ay < by + bh) {
            applyCapeSelection("");
            return;
        }
        for (size_t i = 0; i < _capeChoices.size(); ++i) {
            getCapeBoxRect((int)i, cx, hH, bx, by, bw, bh);
            if (x >= bx && x < bx + bw && ay >= by && ay < by + bh) {
                applyCapeSelection(_capeChoices[i]);
                return;
            }
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

	if (_accountDragging) {
		int mx = Mouse::getX();
		int my = Mouse::getY();
		toGUICoordinate(mx, my);
		int delta = _accountDragStartY - my;
		if (delta > 4 || delta < -4) _accountDragMoved = true;
		if (_accountDragMoved) _accountScrollY = _accountDragStartScroll + delta;
	}

	if(currentOptionPane != NULL)
		currentOptionPane->tick(minecraft);
	super::tick();
	if (selectedCategory == 4) {
		bool loggedIn = !minecraft->options.accountToken.empty();
		if (loggedIn) {
			// Keep in-game name locked to account username
			if (minecraft->options.username != minecraft->options.accountUsername) {
				minecraft->options.username = minecraft->options.accountUsername;
				minecraft->options.save();
				if (minecraft->user) minecraft->user->name = minecraft->options.accountUsername;
			}
		} else if (txtUsername) {
			// Sync username field to options when not logged in
			if (txtUsername->text != minecraft->options.username) {
				minecraft->options.username = txtUsername->text;
				minecraft->options.save();
				if (minecraft->user) minecraft->user->name = txtUsername->text;
			}
		}
	}
}
