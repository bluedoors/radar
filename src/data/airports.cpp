#include "data/airports.h"
static const std::vector<Airport> AIRPORTS = {
    { "YSSY", -33.9461f, 151.1772f },  // Sydney Kingsford Smith
    { "YSBK", -33.9244f, 150.9881f },  // Bankstown
};
const std::vector<Airport>& get_airports() { return AIRPORTS; }
