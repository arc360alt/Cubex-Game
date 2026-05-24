#include "ChatScreen.h"
#include "../Gui.h"
#include "../../Minecraft.h"
#include "../../player/LocalPlayer.h"
#include "../components/TextBox.h"
#include "../../../network/RakNetInstance.h"
#include "../../../network/ServerSideNetworkHandler.h"
#include "../../../network/packet/ChatPacket.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../SharedConstants.h"
#include "../../../util/StringUtils.h"

ChatScreen::ChatScreen()
	: textBox(NULL), caretTick(0) {
}

ChatScreen::~ChatScreen() {
	delete textBox;
}

void ChatScreen::init() {
	super::init();
	textBox = new TextBox(0, "");
	textBox->w = width - 8;
	textBox->h = 18;
	textBoxes.push_back(textBox);
	textBox->setFocus(minecraft);
}

void ChatScreen::setupPositions() {
	if (!textBox)
		return;

	textBox->x = 4;
	textBox->y = height - 22;
	textBox->w = width - 8;
}

void ChatScreen::tick() {
	++caretTick;
}

bool ChatScreen::handleBackEvent(bool isDown) {
	if (!isDown)
		minecraft->setScreen(NULL);
	return true;
}

void ChatScreen::sendMessage() {
	if (!textBox)
		return;

	std::string message = Util::stringTrim(textBox->text);
	if (!message.empty()) {
		if ((int)message.size() > SharedConstants::MaxChatLength)
			message = message.substr(0, SharedConstants::MaxChatLength);

		bool handledLocally = false;
		if (minecraft->isOnline() && minecraft->raknetInstance->isServer() && !minecraft->isOnlineClient()) {
			ServerSideNetworkHandler* serverHandler = dynamic_cast<ServerSideNetworkHandler*>(minecraft->netCallback);
			if (serverHandler && minecraft->player) {
				ChatPacket packet(message, false);
				serverHandler->handle(minecraft->player->owner, &packet);
				handledLocally = true;
			}
		}

		if (!handledLocally) {
			if (minecraft->isOnline()) {
				ChatPacket packet(message, false);
				minecraft->raknetInstance->send(packet);
			} else if (minecraft->player) {
				minecraft->gui.addMessage("<" + minecraft->player->name + "> " + message);
			}
		}
	}

	minecraft->setScreen(NULL);
}

void ChatScreen::keyPressed(int eventKey) {
	if (eventKey == Keyboard::KEY_ESCAPE) {
		minecraft->setScreen(NULL);
		return;
	}

	if (eventKey == Keyboard::KEY_RETURN) {
		sendMessage();
		return;
	}

	if (eventKey == Keyboard::KEY_BACKSPACE) {
		if (textBox && !textBox->text.empty())
			textBox->text.erase(textBox->text.size() - 1, 1);
		return;
	}
}

void ChatScreen::keyboardNewChar(char inputChar) {
	if (!textBox)
		return;

	if (inputChar >= 32 && inputChar < 127 && (int)textBox->text.size() < SharedConstants::MaxChatLength)
		textBox->text += inputChar;
}

void ChatScreen::mouseClicked(int x, int y, int buttonNum) {
	super::mouseClicked(x, y, buttonNum);
	if (!textBox)
		return;

	const bool inside = x >= textBox->x && x < textBox->x + textBox->w && y >= textBox->y && y < textBox->y + textBox->h;
	if (inside)
		textBox->setFocus(minecraft);
}
void ChatScreen::render(int xm, int ym, float a)
{
	fill(2, height - 58, width - 2, height - 2, 0x90000000);
	font->drawShadow("Chat", 4, (float)(height - 56), 0xffffffff);
	font->drawShadow("Enter to send, Esc to cancel", 4, (float)(height - 46), 0xffc0c0c0);
	minecraft->gui.renderChatMessages(height - 24, 20, true, font);
	if (textBox)
		textBox->render(minecraft, xm, ym);
}
