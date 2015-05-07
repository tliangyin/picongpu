/**
 * Copyright 2013-2015 Axel Huebl, Heiko Burau, Rene Widera, Richard Pausch
 *
 * This file is part of PIConGPU.
 *
 * PIConGPU is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PIConGPU is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PIConGPU.
 * If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once

#include "types.h"
#include "simulation_defines.hpp"

namespace picongpu
{
    /** plane wave (use periodic boundaries!)
     *
     *  no transverse spacial envelope
     *  based on the electric potential
     *  Phi = Phi_0 * exp(0.5 * (x-x_0)^2 / sigma^2) * cos(k*(x - x_0) - phi)
     *  by applying grad Phi = d/dx Phi = E(x)
     *  we get:
     *  E = Phi_0 * exp(0.5 * (x-x_0)^2 / sigma^2) * [k*sin(k*(x - x_0) - phi) + x/sigma^2 * cos(k*(x - x_0) - phi)]
     *
     *  This approach ensures that int_{-infinity}^{+infinity} E(x) = 0 for any phase
     *  if we have no transverse profile as we have with this plane wave train
     *
     *  Since PIConGPU requires a temporally defined electric field, we use:
     *  t = x/c and (x-x_0)/sigma = (t-t_0)/tau and k*(x-x_0) = omega*(t-t_0) with omega/k = c and tau * c = sigma
     *  and get:
     *  E = Phi_0*omega/c * exp(0.5 * (t-t_0)^2 / tau^2) * [sin(omega*(t - t_0) - phi) + t/(omega*tau^2) * cos(omega*(t - t_0) - phi)]
     *  and define Phi_0*omega/c = E_0
     *
     *  Please consider:
     *  The above formulae does only apply to a Gaussian envelope. If the plateau length is
     *  not zero, the integral over the volume will only vanish if the plateau length is
     *  a multiple of the wavelength,
     */
    namespace laserPlaneWave
    {
        /** calculates longitudinal field distribution
         *
         * @param currentStep
         * @param phase
         * @return
         */
        HINLINE float3_X laserLongitudinal( uint32_t currentStep, float_X& phase )
        {
            const double runTime = DELTA_T*currentStep;
            const double f = SPEED_OF_LIGHT / WAVE_LENGTH;

            double envelope = double(AMPLITUDE );
            float3_X elong(float3_X::create(0.0));

            const double mue = 0.5 * RAMP_INIT * PULSE_LENGTH;

            const double w = 2.0 * PI * f;

            const double endUpramp = mue;
            const double startDownramp = mue + LASER_NOFOCUS_CONSTANT;

            double integrationCorrectionFactor = 0.0;

            if( runTime > startDownramp )
            {
                // downramp = end
                const double exponent =
                    ( ( runTime - startDownramp )
                      / PULSE_LENGTH / sqrt( 2.0 ) );
                envelope *= exp( -0.5 * exponent * exponent );
                integrationCorrectionFactor = ( runTime - startDownramp )/ (2.0*PULSE_LENGTH*PULSE_LENGTH);
            }
            else if ( runTime < endUpramp )
            {
                // upramp = start
                const double exponent = ( ( runTime - endUpramp ) / PULSE_LENGTH / sqrt( 2.0 ) );
                envelope *= exp( -0.5 * exponent * exponent );
                integrationCorrectionFactor = ( runTime - endUpramp )/ (2.0*PULSE_LENGTH*PULSE_LENGTH);
            }

            const double timeOszi = runTime - endUpramp;
            const double t_and_phase = w * timeOszi + LASER_PHASE;
            // to understand both components [sin(...) + t/tau^2 * cos(...)] see description above
            if( Polarisation == LINEAR_X )
            {
              elong.x() = float_X( envelope * (math::sin(t_and_phase)
                          + math::cos(t_and_phase) * integrationCorrectionFactor));
            }
            else if( Polarisation == LINEAR_Z)
            {
              elong.z() = float_X( envelope * (math::sin(t_and_phase)
                          + math::cos(t_and_phase) * integrationCorrectionFactor));
            }
            else if( Polarisation == CIRCULAR )
            {
                elong.x() = float_X( envelope / sqrt(2.0) * (math::sin(t_and_phase)
                            + math::cos(t_and_phase) * integrationCorrectionFactor));
                elong.z() = float_X( envelope / sqrt(2.0) * (math::cos(t_and_phase)
                            - math::sin(t_and_phase) * integrationCorrectionFactor));
            }


            phase = float_X(0.0);

            return elong;
        }

        /** calculates transverse field distribution
         *
         * @param elong
         * @param phase
         * @param posX
         * @param posZ
         * @return
         */
        HDINLINE float3_X laserTransversal( float3_X elong, const float_X, const float_X, const float_X )
        {
            return elong;
        }

    }
}

