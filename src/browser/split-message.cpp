#include "split-message.hpp"

SplitMessage::SplitMessage(const uint8_t id, const uint8_t size) : id(id), size(size)
{}

SplitMessage::~SplitMessage()
{}

std::string SplitMessage::getData() const
{
	std::string res;
	for (auto m : this->messages) {
		res += m.second;
	}

	return res;
}

bool SplitMessage::dataIsReady() const
{
	for (uint8_t i = 0; i < this->size; i++) {
		try {
			this->messages.at(i);
		}
		catch (std::out_of_range&) {
			return false;
		}
	}
	return true;
}

void SplitMessage::addMessage(const browser_message_t& msg)
{
	// Only insert message if the id and size match; else silent fail
	if (msg.split_text.id == this->id && msg.split_text.max == this->size) {
		this->messages.insert({msg.split_text.count, std::string{msg.split_text.text}});
	}
}
