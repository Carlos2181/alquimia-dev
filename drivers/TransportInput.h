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

#ifndef ALQUIMIA_TRANSPORT_INPUT_H_
#define ALQUIMIA_TRANSPORT_INPUT_H_

// This type stores the metadata for a reactive transport simulation.
typedef struct TransportInput TransportInput;

// Types of coupling for time integration.
typedef enum
{
  TRANSPORT_OPERATOR_SPLIT,
  TRANSPORT_GLOBAL_IMPLICIT
} TransportInputCoupling;

// Creates a new TransportInput object by parsing the given input file.
TransportInput* TransportInput_New(const char* input_file);

// Destroys the given TransportInput object, freeing its resources.
void TransportInput_Free(TransportInput* input);

// The following methods give access to relevant simulation data.
void TransportInput_GetSimParameters(TransportInput* input,
                                     TransportInputCoupling* coupling,
                                     double* t_min, 
                                     double* t_max,
                                     int* max_steps,
                                     double* cfl_factor,
                                     int* order_of_accuracy);

void TransportInput_GetDomain(TransportInput* input,
                              double* x_min, 
                              double* x_max, 
                              int* num_cells);

void TransportInput_GetFlow(TransportInput* input,
                            double* temperature,
                            double* x_velocity);

void TransportInput_GetChemistry(TransportInput* input,
                                 char** chemistry_engine,
                                 char** chemistry_input_file,
                                 char** chemical_ic_name,
                                 char** left_chemical_bc_name,
                                 char** right_chemical_bc_name);

void TransportInput_GetOutput(TransportInput* input,
                              char** filename,
                              char** output_type);

#endif
