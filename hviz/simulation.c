#include <stdio.h>
#include "assert.h"
#include "starspray.h"
#include "simulation.h"
#include "simulator.h"
#include "io.h"

extern Environment env;

void startSimulation()
{
    if (env.simStatus & SIM_RUNNING) return;

#if 1
    if (env.pList.nParticles == 0) return;
    if (env.pList.movingParticleIndex != env.pList.nParticles) return;
    env.simStatus = SIM_RUNNING;
    env.currentTimestep = 1;
    env.loadingTimestep = 1;
    if (startSimulator())
    {
        fprintf(stderr, "Error starting simulation!\n");
        env.simStatus = SIM_STOPPED;
    }
#else
    if (env.pList.movingParticleIndex != env.pList.nParticles) return;

    env.simStatus = SIM_RUNNING;
    env.currentTimestep = 1;
    env.loadingTimestep = 1;

    env.pList.nParticles = 11510; // HACK
    env.pList.movingParticleIndex = env.pList.nParticles;
    unsigned int ci=0;
    for (unsigned int i=0; i < env.pList.nParticles; i++, ci+=4)
    {
        env.pList.clr[ci+0] = starColor[0];
        env.pList.clr[ci+1] = starColor[1];
        env.pList.clr[ci+2] = starColor[2];
        env.pList.clr[ci+3] = starColor[3];
    }
#endif

    fprintf(stderr, "Simulation started.\n");
}

void stopSimulation()
{
    if (env.simStatus == SIM_STOPPED) return;

    stopSimulator();
    env.simStatus = SIM_STOPPED; //assert(0);
    fprintf(stderr, "Simulation stopped.\n");
}

void pauseSimulation()
{
    if (env.simStatus == SIM_RUNNING)
    {
        env.simStatus |= SIM_PAUSED;
        pauseSimulator();
        fprintf(stderr, "Simulation paused.\n");
    }
}

void resumeSimulation()
{
    if (env.simStatus & SIM_PAUSED)
    {
        env.simStatus &= ~SIM_PAUSED;
        resumeSimulator();
        fprintf(stderr, "Simulation resumed.\n");
    }
}

void stepSimulation()
{
    assert(env.simStatus == SIM_RUNNING);

    assert(env.currentTimestep != 0);

    //assert(env.currentTimestep <= env.maxTimesteps);

    if (env.currentTimestep <= env.maxTimesteps)
    {
        switch (loadNextFrameFromSimulator())
        {
            case 0:
                env.currentTimestep++;
                break;
            case -1:
                break;
            default:
                stopSimulation();
                fprintf(stderr, "Simulation finished after %i steps.\n", env.currentTimestep);
                break;
        }

        //fprintf(stderr, "Simulation on step %i\n", env.currentTimestep);
    }
    else
    {
        stopSimulation();
        fprintf(stderr, "Simulation finished after %i steps.\n", env.currentTimestep);
    }
}

