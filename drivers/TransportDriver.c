/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */

//
// Alquimia Copyright (c) 2013-2015, The Regents of the University of California, 
// through Lawrence Berkeley National Laboratory (subject to receipt of any 
// required approvals from the U.S. Dept. of Energy).  All rights reserved.
// 
// Alquimia is available under a BSD license. See LICENSE.txt for more
// information.
//
// If you have questions about your rights to use or distribute this software, 
// please contact Berkeley Lab's Technology Transfer and Intellectual Property 
// Management at TTD@lbl.gov referring to Alquimia (LBNL Ref. 2013-119).
// 
// NOTICE.  This software was developed under funding from the U.S. Department 
// of Energy.  As such, the U.S. Government has been granted for itself and 
// others acting on its behalf a paid-up, nonexclusive, irrevocable, worldwide 
// license in the Software to reproduce, prepare derivative works, and perform 
// publicly and display publicly.  Beginning five (5) years after the date 
// permission to assert copyright is obtained from the U.S. Department of Energy, 
// and subject to any subsequent five (5) year renewals, the U.S. Government is 
// granted for itself and others acting on its behalf a paid-up, nonexclusive, 
// irrevocable, worldwide license in the Software to reproduce, prepare derivative
// works, distribute copies to the public, perform publicly and display publicly, 
// and to permit others to do so.
//

#include <limits.h>
#include "alquimia/alquimia_interface.h"
#include "alquimia/alquimia_memory.h"
#include "alquimia/alquimia_util.h"
#include "ini.h"
#include "TransportDriver.h"

static inline double Min(double a, double b)
{
  return (a < b) ? a : b;
}

static inline double Max(double a, double b)
{
  return (a >= b) ? a : b;
}

// Input parsing stuff. See https://github.com/benhoyt/inih for details.
static int ParseInput(void* user,
                      const char* section,
                      const char* name,
                      const char* value)
{
  TransportDriverInput* input = user;
#define MATCH(s, n) (strcmp(section, s) == 0) && (strcmp(name, n) == 0)

  // Simulation section
  if (MATCH("simulation","t_min"))
    input->t_min = atof(value);
  else if (MATCH("simulation","t_max"))
    input->t_max = atof(value);
  else if (MATCH("simulation","max_steps"))
    input->max_steps = atoi(value);
  else if (MATCH("simulation", "cfl_factor"))
    input->cfl_factor = atof(value);

  // Domain section.
  else if (MATCH("domain", "x_min"))
    input->x_min = atof(value);
  else if (MATCH("domain", "x_max"))
    input->x_max = atof(value);
  else if (MATCH("domain", "num_cells"))
    input->num_cells = atoi(value);

  // Material section.
  else if (MATCH("material", "porosity"))
    input->porosity = atof(value);
  else if (MATCH("material", "saturation"))
    input->saturation = atof(value);

  // Flow section.
  else if (MATCH("flow", "temperature"))
    input->temperature = atof(value);
  else if (MATCH("flow", "velocity"))
    input->velocity = atof(value);

  // Chemistry section.
  else if (MATCH("chemistry", "engine"))
    input->chemistry_engine = AlquimiaStringDup(value);
  else if (MATCH("chemistry", "input_file"))
    input->chemistry_input_file = AlquimiaStringDup(value);
  else if (MATCH("chemistry", "initial_condition"))
    input->ic_name = AlquimiaStringDup(value);
  else if (MATCH("chemistry", "left_boundary_condition"))
    input->left_bc_name = AlquimiaStringDup(value);
  else if (MATCH("chemistry", "right_boundary_condition"))
    input->right_bc_name = AlquimiaStringDup(value);

  // Output section.
  else if (MATCH("output", "type"))
    input->output_type = AlquimiaStringDup(value);
  else if (MATCH("output", "filename"))
    input->output_file = AlquimiaStringDup(value);

  return 1;
}

TransportDriverInput* TransportDriverInput_New(const char* input_file)
{
  TransportDriverInput* input = malloc(sizeof(TransportDriverInput));
  memset(input, 0, sizeof(TransportDriverInput));

  // Make sure we have some meaningful defaults.
  input->t_min = 0.0;
  input->max_steps = INT_MAX;
  input->cfl_factor = 1.0;
  input->porosity = 1.0;
  input->saturation = 1.0;
  input->temperature = 25.0;
  input->velocity = 0.0;

  // Fill in fields by parsing the input file.
  int error = ini_parse(input_file, ParseInput, input);
  if (error != 0)
  {
    TransportDriverInput_Free(input);
    alquimia_error("TransportDriver: Error parsing input: %s", input_file);
  }

  // Verify that our required fields are filled properly.
  if (input->t_max <= input->t_min)
    alquimia_error("TransportDriver: simulation->t_max must be greater than simulation->t_min.");
  if (input->max_steps <= 0)
    alquimia_error("TransportDriver: simulation->max_steps must be non-negative.");
  if ((input->cfl_factor <= 0.0) || (input->cfl_factor > 1.0))
    alquimia_error("TransportDriver: simulation->cfl_factor must be within (0, 1].");
  if (input->x_max <= input->x_min)
    alquimia_error("TransportDriver: domain->x_max must be greater than domain->x_min.");
  if (input->num_cells <= 0)
    alquimia_error("TransportDriver: domain->num_cells must be positive.");
  if ((input->porosity <= 0.0) || (input->porosity > 1.0))
    alquimia_error("TransportDriver: material->porosity must be within (0, 1].");
  if ((input->saturation <= 0.0) || (input->saturation > 1.0))
    alquimia_error("TransportDriver: material->saturation must be within (0, 1].");
  if (input->temperature <= 0.0)
    alquimia_error("TransportDriver: flow->temperature must be positive.");

  // Default output.
  if (input->output_type == NULL)
    input->output_type = AlquimiaStringDup("gnuplot");
  char default_output_file[FILENAME_MAX];
  if (input->output_file == NULL)
  {
    // Start at the first . we find from the end.
    int i = strlen(input_file)-1;
    while ((i > 0) && (input_file[i] != '.')) --i;
    if (i == 0)
      sprintf(default_output_file, "%s.gnuplot", input_file);
    else
    {
      memcpy(default_output_file, input_file, sizeof(char) * i);
      strcat(default_output_file, ".gnuplot");
    }
    input->output_type = AlquimiaStringDup(default_output_file);
  }

  return input;
}

void TransportDriverInput_Free(TransportDriverInput* input)
{
  if (input->ic_name != NULL)
    free(input->ic_name);
  if (input->left_bc_name != NULL)
    free(input->left_bc_name);
  if (input->right_bc_name != NULL)
    free(input->right_bc_name);
  if (input->chemistry_engine != NULL)
    free(input->chemistry_engine);
  if (input->chemistry_input_file != NULL)
    free(input->chemistry_input_file);
  if (input->output_file != NULL)
    free(input->output_file);
  if (input->output_type != NULL)
    free(input->output_type);
  free(input);
}

struct TransportDriver 
{
  // Simulation parameters.
  TransportCoupling coupling;
  double t_min, t_max, cfl;
  int max_steps;

  // 1D grid information.
  int num_cells;
  double x_min, x_max;

  // Flow velocity, temperature.
  double vx, temperature;

  // Material properties.
  double porosity, saturation;

  // Current sim state.
  double time;
  int step;

  // Per-cell chemistry data.
  AlquimiaProperties* chem_properties;
  AlquimiaState* chem_state;
  AlquimiaAuxiliaryData* chem_aux_data;
  AlquimiaAuxiliaryOutputData* chem_aux_output;

  // Chemistry engine -- one of each of these per thread in general.
  AlquimiaInterface chem;
  void* chem_engine;
  AlquimiaEngineStatus chem_status;

  // Chemistry metadata.
  AlquimiaSizes chem_sizes;
  AlquimiaProblemMetaData chem_metadata;

  // Initial and boundary conditions.
  AlquimiaGeochemicalCondition chem_ic;
  AlquimiaGeochemicalCondition chem_left_bc, chem_right_bc;
  AlquimiaState chem_left_state, chem_right_state;
  AlquimiaAuxiliaryData chem_left_aux_data, chem_right_aux_data;

  // Bookkeeping.
  AlquimiaState advected_chem_state;
  double* advective_fluxes;
};

TransportDriver* TransportDriver_New(TransportDriverInput* input)
{
  TransportDriver* driver = malloc(sizeof(TransportDriver));

  // Get basic simulation parameters.
  driver->coupling = input->coupling;
  driver->t_min = input->t_min; 
  driver->t_max = input->t_max;
  driver->max_steps = input->max_steps;
  driver->cfl = input->cfl_factor;

  // Get grid information.
  driver->x_min = input->x_min; 
  driver->x_max = input->x_max;
  driver->num_cells = input->num_cells;

  // Get material properties.
  driver->porosity = input->porosity;
  driver->saturation = input->saturation;

  // Get flow variables.
  driver->vx = input->velocity;
  driver->temperature = input->temperature;

  // Simulation state.
  driver->time = driver->t_min;
  driver->step = 0;

  // Set up the chemistry engine.
  AllocateAlquimiaEngineStatus(&driver->chem_status);
  CreateAlquimiaInterface(input->chemistry_engine, &driver->chem, &driver->chem_status);
  if (driver->chem_status.error != 0) 
  {
    alquimia_error("TransportDriver_New: %s", driver->chem_status.message);
    return NULL;
  }

  // Set up the engine and get storage requirements.
  AlquimiaEngineFunctionality chem_engine_functionality;
  driver->chem.Setup(input->chemistry_input_file,
                     &driver->chem_engine,
                     &driver->chem_sizes,
                     &chem_engine_functionality,
                     &driver->chem_status);
  if (driver->chem_status.error != 0) 
  {
    alquimia_error("TransportDriver_New: %s", driver->chem_status.message);
    return NULL;
  }

  // If you want multiple copies of the chemistry engine with
  // OpenMP, verify: chem_data.functionality.thread_safe == true,
  // then create the appropriate number of chem status and data
  // objects.

  // Allocate memory for the chemistry data.
  AllocateAlquimiaProblemMetaData(&driver->chem_sizes, &driver->chem_metadata);
  driver->chem_properties = malloc(sizeof(AlquimiaProperties) * driver->num_cells);
  driver->chem_state = malloc(sizeof(AlquimiaState) * driver->num_cells);
  driver->chem_aux_data = malloc(sizeof(AlquimiaAuxiliaryData) * driver->num_cells);
  driver->chem_aux_output = malloc(sizeof(AlquimiaAuxiliaryOutputData) * driver->num_cells);
  for (int i = 0; i < driver->num_cells; ++i)
  {
    AllocateAlquimiaState(&driver->chem_sizes, &driver->chem_state[i]);
    AllocateAlquimiaProperties(&driver->chem_sizes, &driver->chem_properties[i]);
    AllocateAlquimiaAuxiliaryData(&driver->chem_sizes, &driver->chem_aux_data[i]);
    AllocateAlquimiaAuxiliaryOutputData(&driver->chem_sizes, &driver->chem_aux_output[i]);
  }

  // Initial condition.
  AllocateAlquimiaGeochemicalCondition(strlen(input->ic_name), 0, 0, &driver->chem_ic);
  strcpy(driver->chem_ic.name, input->ic_name);

  // Boundary conditions.
  AllocateAlquimiaGeochemicalCondition(strlen(input->left_bc_name), 0, 0, &driver->chem_left_bc);
  strcpy(driver->chem_left_bc.name, input->left_bc_name);
  AllocateAlquimiaState(&driver->chem_sizes, &driver->chem_left_state);
  AllocateAlquimiaAuxiliaryData(&driver->chem_sizes, &driver->chem_left_aux_data);
  AllocateAlquimiaGeochemicalCondition(strlen(input->right_bc_name), 0, 0, &driver->chem_right_bc);
  strcpy(driver->chem_right_bc.name, input->right_bc_name);
  AllocateAlquimiaState(&driver->chem_sizes, &driver->chem_right_state);
  AllocateAlquimiaAuxiliaryData(&driver->chem_sizes, &driver->chem_right_aux_data);

  // Bookkeeping.
  AllocateAlquimiaState(&driver->chem_sizes, &driver->advected_chem_state);
  driver->advective_fluxes = malloc(sizeof(double) * driver->chem_sizes.num_primary * (driver->num_cells + 1));

  return driver;
}

void TransportDriver_Free(TransportDriver* driver)
{
  // Destroy advection bookkeeping.
  free(driver->advective_fluxes);
  FreeAlquimiaState(&driver->advected_chem_state);

  // Destroy boundary and initial conditions.
  FreeAlquimiaGeochemicalCondition(&driver->chem_left_bc);
  FreeAlquimiaState(&driver->chem_left_state);
  FreeAlquimiaAuxiliaryData(&driver->chem_left_aux_data);
  FreeAlquimiaGeochemicalCondition(&driver->chem_right_bc);
  FreeAlquimiaState(&driver->chem_right_state);
  FreeAlquimiaAuxiliaryData(&driver->chem_right_aux_data);
  FreeAlquimiaGeochemicalCondition(&driver->chem_ic);

  // Destroy chemistry data.
  for (int i = 0; i < driver->num_cells; ++i)
  {
    FreeAlquimiaState(&driver->chem_state[i]);
    FreeAlquimiaProperties(&driver->chem_properties[i]);
    FreeAlquimiaAuxiliaryData(&driver->chem_aux_data[i]);
    FreeAlquimiaAuxiliaryOutputData(&driver->chem_aux_output[i]);
  }
  free(driver->chem_state);
  free(driver->chem_properties);
  free(driver->chem_aux_data);
  free(driver->chem_aux_output);
  FreeAlquimiaProblemMetaData(&driver->chem_metadata);

  // Destroy chemistry engine.
  driver->chem.Shutdown(&driver->chem_engine, 
                        &driver->chem_status);
  FreeAlquimiaEngineStatus(&driver->chem_status);
  free(driver);
}

static int TransportDriver_Initialize(TransportDriver* driver)
{
  // Determine the volume of a cell from the domain.
  double volume = (driver->x_max - driver->x_min) / driver->num_cells;

  // Initialize each cell.
  for (int i = 0; i < driver->num_cells; ++i)
  {
    // Set the material properties.
    driver->chem_properties[i].volume = volume;
    driver->chem_properties[i].saturation = driver->saturation;

    // Invoke the chemical initial condition.
    driver->chem.ProcessCondition(&driver->chem_engine,
                                  &driver->chem_ic, 
                                  &driver->chem_properties[i],
                                  &driver->chem_state[i],
                                  &driver->chem_aux_data[i],
                                  &driver->chem_status);
    if (driver->chem_status.error != 0)
    {
      printf("TransportDriver: initialization error in cell %d: %s\n", 
             i, driver->chem_status.message);
      break;
    }

    // Overwrite the temperature and porosity in this cell.
    driver->chem_state[i].temperature = driver->temperature;
    driver->chem_state[i].porosity = driver->porosity;
  }

  return driver->chem_status.error;
}

// Here's a finite volume method for advecting concentrations using the 
// 1D flow velocity ux in the given cell.
static int ComputeAdvectiveFluxes(TransportDriver* driver, 
                                  double dx,
                                  double* advective_fluxes)
{
  int num_primary = driver->chem_sizes.num_primary;

  // Estimate the concentrations on each interface, beginning with those 
  // at the boundaries. Note that properties are taken from the leftmost
  // and rightmost cells. Then compute fluxes.

  // Left boundary.
  driver->chem.ProcessCondition(&driver->chem_engine,
                                &driver->chem_left_bc, 
                                &driver->chem_properties[0],
                                &driver->chem_left_state,
                                &driver->chem_left_aux_data,
                                &driver->chem_status);
  if (driver->chem_status.error != 0)
  {
    printf("TransportDriver: boundary condition error at leftmost interface: %s\n",
           driver->chem_status.message);
    return driver->chem_status.error;
  }
  double phi_l = driver->chem_left_state.porosity;
  for (int c = 0; c < num_primary; ++c)
    advective_fluxes[c] = phi_l * driver->vx * driver->chem_left_state.total_mobile.data[c];
  // Right boundary.
  driver->chem.ProcessCondition(&driver->chem_engine,
                                &driver->chem_right_bc, 
                                &driver->chem_properties[driver->num_cells-1],
                                &driver->chem_right_state,
                                &driver->chem_right_aux_data,
                                &driver->chem_status);
  if (driver->chem_status.error != 0)
  {
    printf("TransportDriver: boundary condition error at rightmost interface: %s\n",
           driver->chem_status.message);
    return driver->chem_status.error;
  }
  double phi_r = driver->chem_right_state.porosity;
  for (int c = 0; c < num_primary; ++c)
    advective_fluxes[num_primary*driver->num_cells+c] = phi_r * driver->vx * driver->chem_right_state.total_mobile.data[c];

  // Interior interfaces.
  for (int i = 1; i < driver->num_cells; ++i)
  {
    // We use the method of Gupta et al, 1991 to construct an explicit, 
    // third-order TVD estimate of the concentration at the interface.
    for (int c = 0; c < num_primary; ++c)
    {
      // Figure out the upstream and downstream concentrations.
      double up_conc, down_conc;
      double conc = driver->chem_state[i].total_mobile.data[c];
      if (driver->vx >= 0.0)
      {
        up_conc = driver->chem_state[i].total_mobile.data[c];
        down_conc = driver->chem_state[i+1].total_mobile.data[c];
      }
      else
      {
        up_conc = driver->chem_state[i+1].total_mobile.data[c];
        down_conc = driver->chem_state[i].total_mobile.data[c];
      }

      // Construct the limiter.
      double r_num = (conc - up_conc) / (2.0 * dx);
      double r_denom = (down_conc - up_conc) / (2.0 * dx);
      double r = r_num / r_denom;
      double beta = Max(0.0, Min(2.0, Min(2.0*r, (2.0 + r) / 3.0)));

      // Compute the interface concentration.
      double interface_conc = conc + 0.5 * beta * (down_conc - conc);

      // Compute the interface porosity.
      double phi = 0.5 * (driver->chem_state[i].porosity + driver->chem_state[i+1].porosity);

      // Compute the flux.
      advective_fluxes[num_primary * i + c] = phi * driver->vx * interface_conc;
    }
  }

  return 0;
}

static int Run_OperatorSplit(TransportDriver* driver)
{
  double dx = (driver->x_max - driver->x_min) / driver->num_cells;
  double dt = driver->cfl * dx / driver->vx;
  int num_primary = driver->chem_sizes.num_primary;

  // Initialize the chemistry state in each cell, and set up the solution vector.
  int status = TransportDriver_Initialize(driver);
  if (status != 0)
    return status;

  driver->time = driver->t_min;
  driver->step = 0;
  while ((driver->time < driver->t_max) && (driver->step < driver->max_steps))
  {
    // Compute the advective fluxes.
    if (driver->vx != 0.0)
    {
      status = ComputeAdvectiveFluxes(driver, dx, driver->advective_fluxes);
      if (status != 0) break;
    }

    for (int i = 0; i < driver->num_cells; ++i)
    {
      double phi = driver->chem_state[i].porosity;

      // Advect the state in this cell using a finite volume method with 
      // the advective fluxes we've computed.
      CopyAlquimiaState(&driver->chem_state[i], &driver->advected_chem_state);
      for (int c = 0; c < num_primary; ++c)
      {
        double F_right = driver->advective_fluxes[num_primary*(i+1)+c];
        double F_left = driver->advective_fluxes[num_primary*i+c];
        double phiC = phi * driver->advected_chem_state.total_mobile.data[c] -
                      (F_right - F_left) / dx;
        driver->advected_chem_state.total_mobile.data[c] = phiC / phi;
      }

      // Do the chemistry step, using the advected state as input.
      driver->chem.ReactionStepOperatorSplit(&driver->chem_engine,
                                             dt, &driver->chem_properties[i],
                                             &driver->advected_chem_state,
                                             &driver->chem_aux_data[i],
                                             &driver->chem_status);
      if (driver->chem_status.error != 0)
      {
        status = driver->chem_status.error;
        printf("TransportDriver: operator-split reaction in cell %d: %s\n", 
               i, driver->chem_status.message);
        break;
      }

      // Copy the advected/reacted state back into place.
      CopyAlquimiaState(&driver->advected_chem_state, &driver->chem_state[i]);
    }

    if (status != 0) break;

    driver->time += dt;
    driver->step += 1;
  }

  return status;
}

static int Run_GlobalImplicit(TransportDriver* driver)
{
  alquimia_error("Globally implicit transport is not yet supported.");
  return -1;
}

int TransportDriver_Run(TransportDriver* driver)
{
  if (driver->coupling == TRANSPORT_OPERATOR_SPLIT)
    return Run_OperatorSplit(driver);
  else
    return Run_GlobalImplicit(driver);
}

void TransportDriver_GetSoluteAndAuxData(TransportDriver* driver,
                                         double* time,
                                         AlquimiaVectorString* var_names,
                                         AlquimiaVectorDouble* var_data)
{
  // FIXME: Right now we only write out the solution.
  int num_primary = driver->chem_sizes.num_primary;
  AllocateAlquimiaVectorString(num_primary, var_names);
  AllocateAlquimiaVectorDouble(num_primary * driver->num_cells, 
                               var_data);
}

