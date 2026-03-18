#include "fluid_coolprop_properties.h"
#include "fluid_properties.h"

#include <limits>

extern "C" {
#include "CoolPropLib.h"
}

constexpr long k_err_buffer_len = 1000;

C_coolprop_properties::C_coolprop_properties(const std::string& fluid)
    : C_coolprop_properties(fluid, "HEOS")
{
}

C_coolprop_properties::C_coolprop_properties(const std::string& fluid, const std::string& backend)
    : C_fluid_properties(),
    m_fluid(fluid),
    m_backend(backend)
{
    init_handle();
}

C_coolprop_properties::~C_coolprop_properties()
{
    free_handle();
}

void C_coolprop_properties::init_handle()
{
    free_handle();

    long err_code = 0;
    char err_buffer[k_err_buffer_len] = { 0 };

    // Make handle for abstract state
    m_handle = AbstractState_factory(m_backend.c_str(), m_fluid.c_str(), &err_code, err_buffer, k_err_buffer_len);
    if (err_code != 0)
    {
        m_handle = -1;
        return;
    }

    auto get_pair = [](const char* a, const char* b = nullptr) -> long {
        long idx = get_input_pair_index(a);
        if (idx < 0 && b)
            idx = get_input_pair_index(b);
        return idx;
    };

    // Input pairs (CoolProp naming varies across versions)
    m_iTD = get_input_pair_index("DmassT_INPUTS");          // (T [K], rho [kg/m3])
    m_iPT = get_input_pair_index("PT_INPUTS");                          // (P [Pa], T [K])
    m_iPH = get_input_pair_index("HmassP_INPUTS");        // (P [Pa], h [J/kg])
    m_iPS = get_input_pair_index("PSmass_INPUTS");        // (P [Pa], s [J/kg-K])
    m_iHS = get_input_pair_index("HmassSmass_INPUTS");    // (h [J/kg], s [J/kg-K])
    m_iQT = get_input_pair_index("QT_INPUTS");            // (Q [-], T [K]) or (T [K], Q [-]) depending on naming
    m_iPQ = get_input_pair_index("PQ_INPUTS");                          // (P [Pa], Q [-])

    // Output params
    m_iT = get_param_index("T");
    m_iP = get_param_index("P");
    m_iDmass = get_param_index("Dmass");
    m_iUmass = get_param_index("Umass");
    m_iHmass = get_param_index("Hmass");
    m_iSmass = get_param_index("Smass");
    m_iCvmass = get_param_index("Cvmass");
    m_iCpmass = get_param_index("Cpmass");
    m_iQ = get_param_index("Q");
    m_iSpeedSound = get_param_index("speed_of_sound");

    // If any are invalid, render handle unusable
    if (m_iTD < 0 || m_iPT < 0 || m_iPH < 0 || m_iPS < 0 || m_iHS < 0 ||
        m_iQT < 0 || m_iPQ < 0 ||
        m_iT < 0 || m_iP < 0 || m_iDmass < 0 || m_iUmass < 0 ||
        m_iHmass < 0 || m_iSmass < 0 || m_iCvmass < 0 ||
        m_iCpmass < 0 || m_iQ < 0 || m_iSpeedSound < 0)
    {
        free_handle();
    }
}

void C_coolprop_properties::free_handle()
{
    if (m_handle < 0)
    {
        return;
    }

    long err_code = 0;
    char err_buffer[k_err_buffer_len] = { 0 };
    AbstractState_free(m_handle, &err_code, err_buffer, k_err_buffer_len);

    m_handle = -1;
}

int C_coolprop_properties::update_handle_state(long input_pair, double val1, double val2)
{
    if (m_handle < 0)
        return -1;

    long err_code = 0;
    char err_buffer[k_err_buffer_len] = { 0 };
    AbstractState_update(m_handle, input_pair, val1, val2, &err_code, err_buffer, k_err_buffer_len);

    return (int)err_code;
}

int C_coolprop_properties::fluid_state_from_handle(fluid_state* fs)
{
    if (m_handle < 0)
        return -1;

    long err_code = 0;
    char err_buffer[k_err_buffer_len] = { 0 };

    const double T = AbstractState_keyed_output(m_handle, m_iT, &err_code, err_buffer, k_err_buffer_len);
    if (err_code != 0)
        return (int)err_code;

    const double P_Pa = AbstractState_keyed_output(m_handle, m_iP, &err_code, err_buffer, k_err_buffer_len);
    if (err_code != 0)
        return (int)err_code;

    fs->temp = T;
    fs->pres = P_Pa / 1000.0; // kPa
    fs->dens = AbstractState_keyed_output(m_handle, m_iDmass, &err_code, err_buffer, k_err_buffer_len);
    fs->qual = AbstractState_keyed_output(m_handle, m_iQ, &err_code, err_buffer, k_err_buffer_len);

    // Convert to kJ/kg to match other TCS fluids
    fs->inte = AbstractState_keyed_output(m_handle, m_iUmass, &err_code, err_buffer, k_err_buffer_len) / 1000.0;
    fs->enth = AbstractState_keyed_output(m_handle, m_iHmass, &err_code, err_buffer, k_err_buffer_len) / 1000.0;
    fs->entr = AbstractState_keyed_output(m_handle, m_iSmass, &err_code, err_buffer, k_err_buffer_len) / 1000.0;
    fs->cv = AbstractState_keyed_output(m_handle, m_iCvmass, &err_code, err_buffer, k_err_buffer_len) / 1000.0;
    fs->cp = AbstractState_keyed_output(m_handle, m_iCpmass, &err_code, err_buffer, k_err_buffer_len) / 1000.0;
    fs->ssnd = AbstractState_keyed_output(m_handle, m_iSpeedSound, &err_code, err_buffer, k_err_buffer_len);

    // Saturated densities at temperature T (if supported)
    fs->sat_vap_dens = std::numeric_limits<double>::quiet_NaN();
    fs->sat_liq_dens = std::numeric_limits<double>::quiet_NaN();

    // Vapour (Q=1)
    AbstractState_update(m_handle, m_iQT, 1.0, T, &err_code, err_buffer, k_err_buffer_len);
    if (err_code == 0)
    {
        fs->sat_vap_dens = AbstractState_keyed_output(m_handle, m_iDmass, &err_code, err_buffer, k_err_buffer_len);
    }

    // Liquid (Q=0)
    err_code = 0;
    AbstractState_update(m_handle, m_iQT, 0.0, T, &err_code, err_buffer, k_err_buffer_len);
    if (err_code == 0)
    {
        fs->sat_liq_dens = AbstractState_keyed_output(m_handle, m_iDmass, &err_code, err_buffer, k_err_buffer_len);
    }

    // Restore original state using (P,T)
    err_code = 0;
    AbstractState_update(m_handle, m_iPT, P_Pa, T, &err_code, err_buffer, k_err_buffer_len);

    return 0;
}

int C_coolprop_properties::TD(double T, double D, fluid_state* state)
{
    const int err_code = update_handle_state(m_iTD, T, D);
    if (err_code == 0)
        return fluid_state_from_handle(state);

    return err_code;
}

int C_coolprop_properties::TP(double T, double P, fluid_state* state)
{
    // T [K], P [kPa] in TCS
    const int err_code = update_handle_state(m_iPT, P * 1.e3, T);
    if (err_code == 0)
        return fluid_state_from_handle(state);

    return err_code;
}

int C_coolprop_properties::PH(double P, double H, fluid_state* state)
{
    // P [kPa], H [kJ/kg]
    const int err_code = update_handle_state(m_iPH, P * 1.e3, H * 1.e3);
    if (err_code == 0)
        return fluid_state_from_handle(state);

    return err_code;
}

int C_coolprop_properties::PS(double P, double S, fluid_state* state)
{
    // P [kPa], S [kJ/kg-K]
    const int err_code = update_handle_state(m_iPS, P * 1.e3, S * 1.e3);
    if (err_code == 0)
        return fluid_state_from_handle(state);

    return err_code;
}

int C_coolprop_properties::HS(double H, double S, fluid_state* state)
{
    // H [kJ/kg], S [kJ/kg-K]
    const int err_code = update_handle_state(m_iHS, H * 1.e3, S * 1.e3);
    if (err_code == 0)
        return fluid_state_from_handle(state);

    return err_code;
}

int C_coolprop_properties::TQ(double T, double Q, fluid_state* state)
{
    const int err_code = update_handle_state(m_iQT, Q, T);
    if (err_code == 0)
        return fluid_state_from_handle(state);

    return err_code;
}

int C_coolprop_properties::PQ(double P, double Q, fluid_state* state)
{
    const int err_code = update_handle_state(m_iPQ, P * 1.e3, Q);
    if (err_code == 0)
        return fluid_state_from_handle(state);

    return err_code;
}

double C_coolprop_properties::visc(double D, double T)
{
    if (m_handle < 0)
        return std::numeric_limits<double>::quiet_NaN();

    // Preserve current state using (P,T)
    long err_code = 0;
    char err_buffer[k_err_buffer_len] = { 0 };
    const double T_prev = AbstractState_keyed_output(m_handle, m_iT, &err_code, err_buffer, k_err_buffer_len);
    err_code = 0;
    const double P_prev = AbstractState_keyed_output(m_handle, m_iP, &err_code, err_buffer, k_err_buffer_len);

    // Update to requested T,D
    if (update_handle_state(m_iTD, T, D) != 0)
        return std::numeric_limits<double>::quiet_NaN();

    // CoolProp uses "V" for dynamic viscosity [Pa*s]
    const long iVisc = get_param_index("V");
    if (iVisc < 0)
        return std::numeric_limits<double>::quiet_NaN();

    err_code = 0;
    const double mu_Pa_s = AbstractState_keyed_output(m_handle, iVisc, &err_code, err_buffer, k_err_buffer_len);

    // Restore previous state
    if (T_prev == T_prev && P_prev == P_prev)
    {
        err_code = 0;
        AbstractState_update(m_handle, m_iPT, P_prev, T_prev, &err_code, err_buffer, k_err_buffer_len);
    }

    if (err_code != 0)
        return std::numeric_limits<double>::quiet_NaN();

    // TCS expects [uPa-s]
    return mu_Pa_s * 1.e6;
}

double C_coolprop_properties::cond(double D, double T)
{
    if (m_handle < 0)
        return std::numeric_limits<double>::quiet_NaN();

    // Preserve current state using (P,T)
    long err_code = 0;
    char err_buffer[k_err_buffer_len] = { 0 };
    const double T_prev = AbstractState_keyed_output(m_handle, m_iT, &err_code, err_buffer, k_err_buffer_len);
    err_code = 0;
    const double P_prev = AbstractState_keyed_output(m_handle, m_iP, &err_code, err_buffer, k_err_buffer_len);

    // Update to requested T,D
    if (update_handle_state(m_iTD, T, D) != 0)
        return std::numeric_limits<double>::quiet_NaN();

    // CoolProp uses "L" for thermal conductivity [W/m/K]
    const long iCond = get_param_index("L");
    if (iCond < 0)
        return std::numeric_limits<double>::quiet_NaN();

    err_code = 0;
    const double k_W_mK = AbstractState_keyed_output(m_handle, iCond, &err_code, err_buffer, k_err_buffer_len);

    // Restore previous state
    if (T_prev == T_prev && P_prev == P_prev)
    {
        err_code = 0;
        AbstractState_update(m_handle, m_iPT, P_prev, T_prev, &err_code, err_buffer, k_err_buffer_len);
    }

    if (err_code != 0)
        return std::numeric_limits<double>::quiet_NaN();

    return k_W_mK;
}

void C_coolprop_properties::get_info(fluid_info* info)
{
    if (!info)
        return;

    // Defaults to NaN already, but explicitly reset to avoid leaving stale values
    *info = fluid_info{};

    // Many of these are available via Props1SI (no state needed)
    const double molar_mass = Props1SI(m_fluid.c_str(), "molar_mass");   // [kg/mol]
    const double Tcrit = Props1SI(m_fluid.c_str(), "Tcrit");             // [K]
    const double rhocrit = Props1SI(m_fluid.c_str(), "rhomass_critical"); // [kg/m3]
    const double Pcrit_Pa = Props1SI(m_fluid.c_str(), "pcrit");          // [Pa]
    const double Tmin = Props1SI(m_fluid.c_str(), "Tmin");               // [K]
    const double Tmax = Props1SI(m_fluid.c_str(), "Tmax");               // [K]
    const double Pmin_Pa = Props1SI(m_fluid.c_str(), "pmin");            // [Pa]
    const double Pmax_Pa = Props1SI(m_fluid.c_str(), "pmax");            // [Pa]

    if (molar_mass == molar_mass)
        info->molar_mass = molar_mass * 1000.0; // kg/kmol
    if (Tcrit == Tcrit)
        info->T_critical = Tcrit;
    if (rhocrit == rhocrit)
        info->D_critical = rhocrit;
    if (Pcrit_Pa == Pcrit_Pa)
        info->P_critical = Pcrit_Pa / 1000.0; // kPa

    if (Tmin == Tmin)
        info->temp_lower_limit = Tmin;
    if (Tmax == Tmax)
        info->temp_upper_limit = Tmax;
    if (Pmin_Pa == Pmin_Pa)
        info->pres_lower_limit = Pmin_Pa / 1000.0; // kPa
    if (Pmax_Pa == Pmax_Pa)
        info->pres_upper_limit = Pmax_Pa / 1000.0; // kPa

    // Saturation minimums: compute at triple point if available
    const double Ttriple = Props1SI(m_fluid.c_str(), "Ttriple"); // [K]
    if (Ttriple == Ttriple)
    {
        info->sat_temp_min = Ttriple;

        // p_sat at Q=0 (or 1) are equal at saturation
        const double psat_Pa = PropsSI("P", "T", Ttriple, "Q", 0.0, m_fluid.c_str());
        if (psat_Pa == psat_Pa)
            info->sat_pres_min = psat_Pa / 1000.0; // kPa
    }
}
