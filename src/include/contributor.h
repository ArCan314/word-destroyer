#pragma once

#include <string>
#include <fstream>

#include "user.h"

class Player;
class Contributor : public User
{
public:
	Contributor() = default;
	Contributor(std::ifstream &ifs);
	Contributor(Contributor &&other) noexcept : User(other), word_contributed_(other.word_contributed_) {}
	Contributor &operator=(const Contributor &rhs)
	{
		User::operator=(rhs);
		word_contributed_ = rhs.word_contributed_;
		return *this;
	}
	Contributor(const Contributor &other) : User(other), word_contributed_(other.word_contributed_) {}
	Contributor(const std::string name, const std::string pswd, UserType utype)
		: User(name, pswd, utype), word_contributed_(int(0)) {}

	void from_player(const Player &player);

	void inc_word_contributed() noexcept { word_contributed_++; inc_level(); }
	int get_word_contributed() const noexcept { return word_contributed_; }

	bool Save(std::ofstream &ofs) const override;
	bool Load(std::ifstream &ifs) override;

	~Contributor() = default;

	virtual void inc_level() override;

private:
	int word_contributed_ = 0;
};