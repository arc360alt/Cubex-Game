#include "TextBox.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
#include "../GuiComponent.h"
TextBox::TextBox( int id, const std::string& msg )
 : id(0), w(0), h(0), x(0), y(0), text(msg), focused(false) {

}

TextBox::TextBox( int id, int x, int y, const std::string& msg ) 
 : id(id), w(0), h(0), x(x), y(y), text(msg), focused(false) {

}

TextBox::TextBox( int id, int x, int y, int w, int h, const std::string& msg )
 : id(id), w(w), h(h), x(x), y(y), text(msg) {

}

void TextBox::setFocus(Minecraft* minecraft) {
	if(!focused) {
		minecraft->platform()->showKeyboard();
		focused = true;
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

void TextBox::render(Minecraft* minecraft, int xm, int ym) {
    // Background
    GuiComponent::fill(x - 1, y - 1, x + w + 1, y + h + 1, 0xffa0a0a0); // border
    GuiComponent::fill(x, y, x + w, y + h, 0xff000000); // background

    // Text
    std::string display = text;
    if (focused) display += "_"; // cursor
    minecraft->font->drawShadow(display, (float)(x + 2), (float)(y + (h - 8) / 2), 0xffffffff);
	
}
