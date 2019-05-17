#include <random>

enum UserType;
inline int get_round(int level)
{
	if (level < 30)
		return 1;
	else if (level < 50)
		return 2;
	else if (level < 90)
		return 3;
	else
	{
		static std::random_device rd;
		static std::uniform_int_distribution<int> uid(2, 4);
		return uid(rd);
	}
}

inline int get_round_time(int level)
{
	if (level < 20)
		return 1300;
	else if (level < 40)
		return 1500;
	else if (level < 60)
		return 1600;
	else
	{
		static std::random_device rd;
		static std::uniform_int_distribution<int> uid(1500, 2300);
		return uid(rd);
	}
}

long long get_level_up_bound(int level, UserType utype);

double get_gain_exp(int level, double time, int difficulty);

int get_max_level();
int get_max_stage();