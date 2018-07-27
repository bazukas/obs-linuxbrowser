#include "split-message.hpp"

SplitMessage::SplitMessage(const uint8_t id, const uint8_t size) : id(id), size(size), progress(0)
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

void SplitMessage::addMessage(const split_text_message& msg)
{
	// Only insert message if the id and size match; else silent fail
	if (msg.id == this->id && msg.max == this->size) {
		this->messages.insert({msg.count, std::string{msg.text}});
		this->progress++;
	}
}
