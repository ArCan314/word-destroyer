#include <random>

enum UserType;
inline int get_round(int level)
{
	if (level < 50)
		return 1;
	else if (level < 100)
		return 2;
	else if (level < 150)
		return 3;
	else
	{
		static std::random_device rd;
		static std::uniform_int_distribution<int> uid(2, 5);
		return uid(rd);
	}
}

inline int get_round_time(int level)
{
	if (level < 50)
		return 1300 + level * 10;
	else if (level < 100)
		return 1800 + (level - 50) * 10;
	else if (level < 150)
		return 2300 + (level - 100) * 10;
	else
	{
		static std::random_device rd;
		static std::uniform_int_distribution<int> uid(2000 + level * 5, 3000 + level * 5);
		return uid(rd);
	}
}
double get_level_up_bound(int level, UserType utype);

double get_gain_exp(int level, double time, int difficulty);

int get_max_level();
int get_max_stage();