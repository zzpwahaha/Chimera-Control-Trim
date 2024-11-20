#pragma once
#include <ParameterSystem/Expression.h>

enum class StaticAOGrid : size_t
{
	numPERunit = 8,
	numOFunit = 1,
	total = numPERunit * numOFunit
};

struct StaticAOSettings
{
	std::array<Expression, size_t(StaticAOGrid::total)> staticAOs;
	bool ctrlAO;
};