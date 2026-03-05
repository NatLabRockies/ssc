#include "fluid_properties.h"

#include <memory>

#include "fluid_co2_properties.h"
#include "fluid_water_properties.h"

std::unique_ptr<C_fluid_properties> C_fluid_properties::create_fluid_properties(const E_fluid_type type)
{
    switch (type)
    {
        case E_fluid_type::WATER:
            return std::unique_ptr<C_fluid_properties>(new C_water_properties());
        case E_fluid_type::CO2:
            return std::unique_ptr<C_fluid_properties>(new C_co2_properties());
        default:
            return nullptr;
    }
}
