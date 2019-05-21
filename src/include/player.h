#pragma once

#include <fstream>
#include <string>
#include <utility>

#include "user.h"

class Contributor;
class Player : public User
{
public:
    Player() = default;
    Player(std::ifstream &ifs);

	Player(const Player &other) : User(other), exp_(other.exp_), level_passed_(other.level_passed_) {}
	Player(Player &&other) noexcept: User(std::move(other)), exp_(other.exp_), level_passed_(other.level_passed_) {}
	Player &operator=(const Player &rhs)
	{
		User::operator=(rhs);
		exp_ = rhs.exp_;
		level_passed_ = rhs.level_passed_;
		return *this;
	}
    Player(const std::string &name, const std::string &pswd, UserType utype)
        : User(name, pswd, utype), exp_(double(0)), level_passed_(int(0)) {}

	void from_contributor(const Contributor &con);
	void raise_exp(double increasement) { exp_ += increasement; inc_level(); }
    void inc_level_passed() { level_passed_++; }

    double get_exp() const { return exp_; }
    int get_level_passed() const { return level_passed_; }

    virtual bool Save(std::ofstream &ofs) const override;
    virtual bool Load(std::ifstream &ifs) override;

	virtual void inc_level() override;

    ~Player() = default;
private:
    double exp_ = 0.0;
    int level_passed_ = 0;


};