#include "TextBox.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
#include "../../../platform/input/Keyboard.h"
#include "../GuiComponent.h"
#include "../Font.h"
TextBox::TextBox( int id, const std::string& msg )
 : id(0), w(0), h(0), x(0), y(0), text(msg), focused(false), password(false), cursorPos(-1) {

}

TextBox::TextBox( int id, int x, int y, const std::string& msg ) 
 : id(id), w(0), h(0), x(x), y(y), text(msg), focused(false), password(false), cursorPos(-1) {

}

TextBox::TextBox( int id, int x, int y, int w, int h, const std::string& msg )
 : id(id), w(w), h(h), x(x), y(y), text(msg), focused(false), password(false), cursorPos(-1) {

}

void TextBox::setFocus(Minecraft* minecraft) {
	if(!focused) {
		minecraft->platform()->showKeyboard();
		focused = true;
		cursorPos = -1;
	}
}

bool TextBox::loseFocus(Minecraft* minecraft) {
	if(focused) {
		minecraft->platform()->showKeyboard();
		focused = false;
		return true;
	}
	return false;
}

int TextBox::getCursor() const {
	// text may be assigned directly by screens, so clamp on every use
	if (cursorPos < 0 || cursorPos > (int)text.size()) return (int)text.size();
	return cursorPos;
}

bool TextBox::insertChar(char c, int maxLength) {
	if ((int)text.size() >= maxLength) return false;
	int cur = getCursor();
	text.insert(text.begin() + cur, c);
	if (cursorPos >= 0) cursorPos = cur + 1;
	return true;
}

bool TextBox::handleEditKey(int key) {
	int cur = getCursor();
	int len = (int)text.size();
	switch (key) {
		case Keyboard::KEY_BACKSPACE:
			if (cur > 0) {
				text.erase(cur - 1, 1);
				if (cursorPos >= 0) cursorPos = cur - 1;
			}
			return true;
		case Keyboard::KEY_DELETE:
			if (cur < len) text.erase(cur, 1);
			return true;
		case Keyboard::KEY_LEFT:
			if (cur > 0) cursorPos = cur - 1;
			return true;
		case Keyboard::KEY_RIGHT:
			cursorPos = (cur + 1 >= len) ? -1 : cur + 1;
			return true;
		case Keyboard::KEY_HOME:
			cursorPos = 0;
			return true;
		case Keyboard::KEY_END:
			cursorPos = -1;
			return true;
	}
	return false;
}

void TextBox::render(Minecraft* minecraft, int xm, int ym) {
    // Background
    GuiComponent::fill(x - 1, y - 1, x + w + 1, y + h + 1, 0xffa0a0a0); // border
    GuiComponent::fill(x, y, x + w, y + h, 0xff000000); // background

    // Text
    std::string display = password ? std::string(text.size(), '*') : text;
    float tx = (float)(x + 2);
    float ty = (float)(y + (h - 8) / 2);
    int cur = getCursor();
    if (focused && cur == (int)display.size()) display += "_"; // cursor at end
    minecraft->font->drawShadow(display, tx, ty, 0xffffffff);

    if (focused && cur < (int)text.size()) {
        // cursor mid-text: thin vertical bar before the character
        int cx = (int)tx + minecraft->font->width(display.substr(0, cur));
        GuiComponent::fill(cx, (int)ty - 1, cx + 1, (int)ty + 9, 0xffd0d0d0);
    }
}
