#pragma once
#include <ParameterSystem/Expression.h>

enum class StaticDDSGrid : size_t
{
	numPERunit = 1,
	numOFunit = 1,
	total = numPERunit * numOFunit
};

struct StaticDDSSettings
{
	std::array<Expression, size_t(StaticDDSGrid::total)> staticDDSs;
	bool ctrlDDS;
};