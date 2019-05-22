#pragma once

#include <string>
#include <fstream>

enum UserType
{
    USERTYPE_C = 0,
    USERTYPE_P,
    USERTYPE_U
};

class User
{
public:
    User() = default;
    User(const std::string &name, const std::string &pswd, UserType utype)
        : name_(name), pswd_(pswd), level_(unsigned(0)), user_type_(utype) {}
	User::User(User &&other) noexcept : name_(std::move(other.name_)), pswd_(std::move(other.pswd_)), level_(other.level_), user_type_(other.user_type_) {}
	User::User(const User &other) : name_(other.name_), pswd_(other.pswd_), level_(other.level_), user_type_(other.user_type_) {}
	User &operator=(const User &rhs)
	{
		name_ = rhs.name_;
		pswd_ = rhs.pswd_;
		level_ = rhs.level_;
		user_type_ = rhs.user_type_;
		return *this;
	}
    User(std::ifstream &ifs);

    const std::string &get_password() const { return pswd_; }
    const std::string &get_user_name() const { return name_; }
    void set_level(unsigned level) { level_ = level;}
    void set_user_type(UserType u) { user_type_ = u; }
    UserType get_user_type() const { return user_type_; }

    unsigned get_level() const { return level_; }

	virtual void inc_level() = 0;
    virtual bool Save(std::ofstream &ofs) const;
    virtual bool Load(std::ifstream &ifs);

    virtual ~User() = default;

protected:
    std::string name_;
    std::string pswd_;
    unsigned level_ = 0;
    UserType user_type_ = UserType::USERTYPE_U;

private:
};