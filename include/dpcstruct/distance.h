#pragma once
#include <dpcstruct/types/Alignment.h>
#include <dpcstruct/types/PrimaryCluster.h>
#include <algorithm>
double distance(const Alignment& i, const Alignment& j);
double distance(const SmallPC* i, const SmallPC* j);
