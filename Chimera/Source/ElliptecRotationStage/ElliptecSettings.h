#pragma once
#include <ParameterSystem/Expression.h>

enum class ElliptecGrid : size_t
{
	numPERunit = 2,
	numOFunit = 1,
	total = numPERunit * numOFunit
};

struct ElliptecSettings
{
	std::array<Expression, size_t(ElliptecGrid::total)> elliptecs;
	bool ctrlEll;
};