#ifndef POST_H
#define POST_H

#include "Channel.h"
#include "SolarMax.h"

#include <vector>
#include <string>
#include <memory>

void post(const std::vector<const Channel*> &channels);
void postToPVOutput(const std::string &date, float dailyKWh);

#endif // POST_H
