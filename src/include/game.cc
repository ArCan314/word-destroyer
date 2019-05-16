#include "game.h"
#include "user.h"

static constexpr int max_lv = 200;
static constexpr int max_stage = 120;
static long long player_exp_table[max_lv];
static long long contributor_level_table[max_lv];
static bool is_init = false;

static void init_level_table()
{
	player_exp_table[0] = 15;
	contributor_level_table[0] = 2;

	for (int i = 1; i < max_lv; i++)
	{
		player_exp_table[i] = player_exp_table[i - 1] * 1.2;
		if (i < 10)
			contributor_level_table[i] = contributor_level_table[i - 1] + 2;
		else
			contributor_level_table[i] = contributor_level_table[i - 1] * 1.05;
	}
	is_init = true;
}

long long get_level_up_bound(int level, UserType utype)
{
	if (!is_init)
	{
		init_level_table();
	}
	if (utype == USERTYPE_C)
		return contributor_level_table[level];
	else
		return player_exp_table[level];
	return 0;
}

double get_gain_exp(int level, double time, int difficulty)
{
	return std::pow(1.02, level + 20000.0 / (time + 1000))  + std::pow(1.2 , difficulty) ;
}

int get_max_level()
{
	return max_lv;
}

int get_max_stage()
{
	return max_stage;
}

