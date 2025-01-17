#include "mac_grid.h"
#include "open_gl_headers.h" 
#include "camera.h"
#include "custom_output.h" 
#include "constants.h" 
#include <math.h>
#include <map>
#include <stdio.h>
#include <cstdlib>
#undef max
#undef min 
#include <fstream> 


// Globals
MACGrid target;


// NOTE: x -> cols, z -> rows, y -> stacks
MACGrid::RenderMode MACGrid::theRenderMode = SHEETS; // { CUBES; SHEETS; }
MACGrid::BackTraceMode MACGrid::theBackTraceMode = RK2; // { FORWARDEULER, RK2 };
MACGrid::SourceType MACGrid::theSourceType = CUBECENTER; // { INIT, CUBECENTER, TWOSOURCE };
bool MACGrid::theDisplayVel = false; //true

#define FOR_EACH_CELL \
   for(int k = 0; k < theDim[MACGrid::Z]; k++)  \
      for(int j = 0; j < theDim[MACGrid::Y]; j++) \
         for(int i = 0; i < theDim[MACGrid::X]; i++) 

#define FOR_EACH_CELL_REVERSE \
   for(int k = theDim[MACGrid::Z] - 1; k >= 0; k--)  \
      for(int j = theDim[MACGrid::Y] - 1; j >= 0; j--) \
         for(int i = theDim[MACGrid::X] - 1; i >= 0; i--) 

#define FOR_EACH_FACE \
   for(int k = 0; k < theDim[MACGrid::Z]+1; k++) \
      for(int j = 0; j < theDim[MACGrid::Y]+1; j++) \
         for(int i = 0; i < theDim[MACGrid::X]+1; i++) 


#define FOR_EACH_YFACE \
   for(int k = 0; k < theDim[MACGrid::Z]; k++) \
      for(int j = 0; j < theDim[MACGrid::Y]+1; j++) \
         for(int i = 0; i < theDim[MACGrid::X]; i++)



MACGrid::MACGrid()
{
   initialize();
}

MACGrid::MACGrid(const MACGrid& orig)
{
   mU = orig.mU;
   mV = orig.mV;
   mW = orig.mW;
   mP = orig.mP;
   mD = orig.mD;
   mT = orig.mT;
}

MACGrid& MACGrid::operator=(const MACGrid& orig)
{
   if (&orig == this)
   {
      return *this;
   }
   mU = orig.mU;
   mV = orig.mV;
   mW = orig.mW;
   mP = orig.mP;
   mD = orig.mD;
   mT = orig.mT;   

   return *this;
}

MACGrid::~MACGrid()
{
}

void MACGrid::reset()
{
   mU.initialize();
   mV.initialize();
   mW.initialize();
   mP.initialize();
   mD.initialize();
   mT.initialize(0.0);

    if(useEigen)
        calculateEigenAMatrix();
    else {
        calculateAMatrix();
        calculatePreconditioner(AMatrix);
    }

}

void MACGrid::initialize()
{
   reset();
}

void MACGrid::updateSources()
{
    // Set initial values for density, temperature, velocity

    if(theSourceType == INIT) {
        // used in [32, 32, 1] grid
        for (int i = 6; i < 12; i++) {
            for (int j = 0; j < 5; j++) {
                mV(i, j + 1, 0) = 2.0;
                mD(i, j, 0) = 1.0;
                mT(i, j, 0) = 1.0;

                mV(i, j + 2, 0) = 2.0;
                mD(i, j, 0) = 1.0;
                mT(i, j, 0) = 1.0;
            }
        }


        // Refresh particles in source.
        for (int i = 6; i < 12; i++) {
            for (int j = 0; j < 5; j++) {
                for (int k = 0; k <= 0; k++) {
                    vec3 cell_center(theCellSize * (i + 0.5), theCellSize * (j + 0.5), theCellSize * (k + 0.5));
                    for (int p = 0; p < 10; p++) {
                        double a = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                        double b = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                        double c = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                        vec3 shift(a, b, c);
                        vec3 xp = cell_center + shift;
                        rendering_particles.push_back(xp);
                    }
                }
            }
        }

    } // end INIT

    else if(theSourceType == CUBECENTER) {
        if(theDim[0] == 32) {
            // used in [32, 32, 32] grid
            for (int i = 12; i < 20; i++) {
                for (int j = 0; j < 1; j++) {
                    for (int k = 12; k < 20; k++) {
                        mV(i, j + 1, k) = 5.0;
                        mD(i, j, k) = 1.0;
                        mT(i, j, k) = 1.0;
                    }
                }
            }

            // Refresh particles in source.
            for (int i = 12; i < 20; i++) {
                for (int j = 0; j < 1; j++) {
                    for (int k = 12; k < 20; k++) {
                        vec3 cell_center(theCellSize * (i + 0.5), theCellSize * (j + 0.5), theCellSize * (k + 0.5));
                        for (int p = 0; p < 10; p++) {
                            double a = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            double b = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            double c = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            vec3 shift(a, b, c);
                            vec3 xp = cell_center + shift;
                            rendering_particles.push_back(xp);
                        }
                    }
                }
            }
        }

        else if(theDim[0] == 64) {
            // used in [64, 64, 64] grid
            for (int i = 20; i < 42; i++) {
                for (int j = 0; j < 2; j++) {
                    for (int k = 26; k < 38; k++) {
                        mV(i, j + 1, k) = 5.0;
                        mD(i, j, k) = 1.0;
                        mT(i, j, k) = 1.0;
                    }
                }
            }

            // Refresh particles in source.
            for (int i = 20; i < 42; i++) {
                for (int j = 0; j < 2; j++) {
                    for (int k = 26; k < 38; k++) {
                        vec3 cell_center(theCellSize * (i + 0.5), theCellSize * (j + 0.5), theCellSize * (k + 0.5));
                        for (int p = 0; p < 10; p++) {
                            double a = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            double b = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            double c = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            vec3 shift(a, b, c);
                            vec3 xp = cell_center + shift;
                            rendering_particles.push_back(xp);
                        }
                    }
                }
            }
        }

        // to test
        else if(theDim[0] == 3) {
            // used in [3, 3, 3] grid
            mV(0, 1, 1) = 1.0;
            mD(0, 0, 1) = 1.0;
            mT(0, 0, 1) = 1.0;
        }

        else if(theDim[0] == 16) {
            // used in [16, 16, 16] grid
            for (int i = 5; i < 13; i++) {
                for (int j = 0; j < 2; j++) {
                    for (int k = 5; k < 13; k++) {
                        mV(i, j + 1, k) = 5.0;
                        mD(i, j, k) = 1.0;
                        mT(i, j, k) = 1.0;
                    }
                }
            }

            // Refresh particles in source.
            for (int i = 5; i < 13; i++) {
                for (int j = 0; j < 2; j++) {
                    for (int k = 5; k < 13; k++) {
                        vec3 cell_center(theCellSize * (i + 0.5), theCellSize * (j + 0.5), theCellSize * (k + 0.5));
                        for (int p = 0; p < 10; p++) {
                            double a = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            double b = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            double c = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            vec3 shift(a, b, c);
                            vec3 xp = cell_center + shift;
                            rendering_particles.push_back(xp);
                        }
                    }
                }
            }
        }
    }

    else if(theSourceType == TWOSOURCE) {
        if(theDim[0] == 32) {
            // used in [32, 32, 32] grid
            for (int i = 0; i < 2; i++) {
                for (int j = 5; j < 7; j++) {
                    for (int k = 15; k < 17; k++) {
                        mU(i, j + 1, k) = 5.0;
                        mD(i, j, k) = 1.0;
                        mT(i, j, k) = 1.0;
                    }
                }
            }

            for (int i = theDim[0] - 2; i < theDim[0]; i++) {
                for (int j = 5; j < 7; j++) {
                    for (int k = 15; k < 17; k++) {
                        mU(i, j + 1, k) = -5.0;
                        mD(i, j, k) = 1.0;
                        mT(i, j, k) = 1.0;
                    }
                }
            }

            // Refresh particles in source.
            for (int i = 0; i < 2; i++) {
                for (int j = 5; j < 7; j++) {
                    for (int k = 15; k < 17; k++) {
                        vec3 cell_center(theCellSize * (i + 0.5), theCellSize * (j + 0.5), theCellSize * (k + 0.5));
                        for (int p = 0; p < 10; p++) {
                            double a = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            double b = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            double c = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            vec3 shift(a, b, c);
                            vec3 xp = cell_center + shift;
                            rendering_particles.push_back(xp);
                        }
                    }
                }
            }

            for (int i = theDim[0] - 2; i < theDim[0]; i++) {
                for (int j = 5; j < 7; j++) {
                    for (int k = 15; k < 17; k++) {
                        vec3 cell_center(theCellSize * (i + 0.5), theCellSize * (j + 0.5), theCellSize * (k + 0.5));
                        for (int p = 0; p < 10; p++) {
                            double a = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            double b = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            double c = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            vec3 shift(a, b, c);
                            vec3 xp = cell_center + shift;
                            rendering_particles.push_back(xp);
                        }
                    }
                }
            }
        }

        else if(theDim[0] == 64) {
            // used in [64, 64, 64] grid
            for (int i = 0; i < 2; i++) {
                for (int j = 10; j < 15; j++) {
                    for (int k = 30; k < 35; k++) {
                        mU(i, j + 1, k) = 5.0;
                        mD(i, j, k) = 1.0;
                        mT(i, j, k) = 1.0;
                    }
                }
            }

            for (int i = theDim[0] - 2; i < theDim[0]; i++) {
                for (int j = 10; j < 15; j++) {
                    for (int k = 30; k < 35; k++) {
                        mU(i, j + 1, k) = -5.0;
                        mD(i, j, k) = 1.0;
                        mT(i, j, k) = 1.0;
                    }
                }
            }

            // Refresh particles in source.
            for (int i = 0; i < 2; i++) {
                for (int j = 10; j < 15; j++) {
                    for (int k = 30; k < 35; k++) {
                        vec3 cell_center(theCellSize * (i + 0.5), theCellSize * (j + 0.5), theCellSize * (k + 0.5));
                        for (int p = 0; p < 10; p++) {
                            double a = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            double b = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            double c = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            vec3 shift(a, b, c);
                            vec3 xp = cell_center + shift;
                            rendering_particles.push_back(xp);
                        }
                    }
                }
            }

            for (int i = theDim[0] - 2; i < theDim[0]; i++) {
                for (int j = 10; j < 15; j++) {
                    for (int k = 30; k < 35; k++) {
                        vec3 cell_center(theCellSize * (i + 0.5), theCellSize * (j + 0.5), theCellSize * (k + 0.5));
                        for (int p = 0; p < 10; p++) {
                            double a = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            double b = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            double c = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                            vec3 shift(a, b, c);
                            vec3 xp = cell_center + shift;
                            rendering_particles.push_back(xp);
                        }
                    }
                }
            }
        }
    }

}


void MACGrid::advectVelocity (double dt)
{
    // TODO: Calculate new velocities and store in target


    // TODO: Get rid of these three lines after you implement yours
	//target.mU = mU;
    //target.mV = mV;
    //target.mW = mW;

    // TODO: Your code is here. It builds target.mU, target.mV and target.mW for all faces
    FOR_EACH_FACE {
        // we have X-face(i,j,k), where i~[0, dim[X], j~[0, dim[Y]-1], k~[0, dim[Z]-1]
                // Y-face(i,j,k), where i~[0, dim[X]-1, j~[0, dim[Y]], k~[0, dim[Z]-1]
                // Z-face(i,j,k), where i~[0, dim[X]-1, j~[0, dim[Y]]-1, k~[0, dim[Z]]

        //std::cout << i << ", " << j << ", " << k << ": " << std::endl;

        if(isValidFace(MACGrid::X, i, j, k)) {
            if(i == 0 || i == theDim[MACGrid::X] || isBoxBoundaryFace(MACGrid::X, i, j, k)) {
                target.mU(i, j, k) = 0;
            }
            else {
                vec3 curPosXface = getFacePosition(MACGrid::X, i, j, k);
                vec3 curVelXface = getVelocity(curPosXface);

                /*
                double curVely = 0, curVelz = 0;

                // Average mV of 4 neighboring Y faces
                if(isValidFace(MACGrid::Y, i-1, j, k)) curVely += mV(i-1, j, k);
                if(isValidFace(MACGrid::Y, i-1, j+1, k)) curVely += mV(i-1, j+1, k);
                if(isValidFace(MACGrid::Y, i, j, k)) curVely += mV(i, j, k);
                if(isValidFace(MACGrid::Y, i, j+1, k)) curVely += mV(i, j+1, k);
                curVely *= 0.25;

                // Average mW of 4 neighboring Z faces
                if(isValidFace(MACGrid::Z, i-1, j, k)) curVelz += mW(i-1, j, k);
                if(isValidFace(MACGrid::Z, i-1, j, k+1)) curVelz += mW(i-1, j, k+1);
                if(isValidFace(MACGrid::Z, i, j, k)) curVelz += mW(i, j, k);
                if(isValidFace(MACGrid::Z, i, j, k+1)) curVelz += mW(i, j, k+1);
                curVelz *= 0.25;

                vec3 curVelXface(mU(i, j, k), curVely, curVelz);*/
                vec3 oldPosXface;

                if (theBackTraceMode == FORWARDEULER) {
                    // Forward Euler
                    oldPosXface = curPosXface - dt * curVelXface;
                } else if (theBackTraceMode == RK2) {
                    // RK2
                    vec3 midPosX = curPosXface - 0.5 * dt * curVelXface;
                    vec3 clippedMidPosX = clipToGrid(midPosX, curPosXface);
                    vec3 midVelX = getVelocity(clippedMidPosX);
                    oldPosXface = curPosXface - dt * midVelX;
                }

                vec3 clippedOldPosX = clipToGrid(oldPosXface, curPosXface);
                vec3 newVelX = getVelocity(clippedOldPosX);
                target.mU(i, j, k) = newVelX[0];
            }
        }

        if(isValidFace(MACGrid::Y, i, j, k)) {
            if(j == 0 || j == theDim[MACGrid::Y] || isBoxBoundaryFace(MACGrid::Y, i, j, k)) {
                target.mV(i, j, k) = 0;
            }
            else {
                vec3 curPosYface = getFacePosition(MACGrid::Y, i, j, k);
                vec3 curVelYface = getVelocity(curPosYface);

                /*
                double curVelx = 0, curVelz = 0;

                // Average mU of 4 neighboring X faces
                if(isValidFace(MACGrid::X, i, j-1, k)) curVelx += mU(i, j-1, k);
                if(isValidFace(MACGrid::X, i+1, j-1, k)) curVelx += mU(i+1, j-1, k);
                if(isValidFace(MACGrid::X, i, j, k)) curVelx += mU(i, j, k);
                if(isValidFace(MACGrid::X, i+1, j, k)) curVelx += mU(i+1, j, k);
                curVelx *= 0.25;

                // Average mW of 4 neighboring Z faces
                if(isValidFace(MACGrid::Z, i-1, j, k)) curVelz += mW(i-1, j, k);
                if(isValidFace(MACGrid::Z, i-1, j, k+1)) curVelz += mW(i-1, j, k+1);
                if(isValidFace(MACGrid::Z, i, j, k)) curVelz += mW(i, j, k);
                if(isValidFace(MACGrid::Z, i, j, k+1)) curVelz += mW(i, j, k+1);
                curVelz *= 0.25;

                vec3 curVelYface(curVelx, mV(i, j, k), curVelz);*/
                vec3 oldPosYface;

                if (theBackTraceMode == FORWARDEULER) {
                    // Forward Euler
                    oldPosYface = curPosYface - dt * curVelYface;
                } else if (theBackTraceMode == RK2) {
                    // RK2
                    vec3 midPosY = curPosYface - 0.5 * dt * curVelYface;
                    vec3 clippedMidPosY = clipToGrid(midPosY, curPosYface);
                    vec3 midVelY = getVelocity(clippedMidPosY);
                    oldPosYface = curPosYface - dt * midVelY;
                }

                vec3 clippedOldPosY = clipToGrid(oldPosYface, curPosYface);
                vec3 newVelY = getVelocity(clippedOldPosY);
                target.mV(i, j, k) = newVelY[1];
            }
        }

        if(isValidFace(MACGrid::Z, i, j, k)) {
            if(k == 0 || k == theDim[MACGrid::Z] || isBoxBoundaryFace(MACGrid::Z, i, j, k)) {
                target.mW(i, j, k) = 0;
            }
            else {
                vec3 curPosZface = getFacePosition(MACGrid::Z, i, j, k);
                vec3 curVelZface = getVelocity(curPosZface);

                /*double curVelx = 0, curVely = 0;

                // Average mU of 4 neighboring X faces
                if(isValidFace(MACGrid::X, i, j-1, k)) curVelx += mU(i, j-1, k);
                if(isValidFace(MACGrid::X, i+1, j-1, k)) curVelx += mU(i+1, j-1, k);
                if(isValidFace(MACGrid::X, i, j, k)) curVelx += mU(i, j, k);
                if(isValidFace(MACGrid::X, i+1, j, k)) curVelx += mU(i+1, j, k);
                curVelx *= 0.25;

                // Average mV of 4 neighboring Y faces
                if(isValidFace(MACGrid::X, i-1, j, k)) curVely += mV(i-1, j, k);
                if(isValidFace(MACGrid::X, i-1, j+1, k)) curVely += mV(i-1, j+1, k);
                if(isValidFace(MACGrid::X, i, j, k)) curVely += mV(i, j, k);
                if(isValidFace(MACGrid::X, i, j+1, k)) curVely += mV(i, j+1, k);
                curVely *= 0.25;

                vec3 curVelZface(curVelx, curVely, mW(i, j, k));*/
                vec3 oldPosZface;

                if (theBackTraceMode == FORWARDEULER) {
                    // Forward Euler
                    oldPosZface = curPosZface - dt * curVelZface;
                } else if (theBackTraceMode == RK2) {
                    // RK2
                    vec3 midPosZ = curPosZface - 0.5 * dt * curVelZface;
                    vec3 clippedMidPosZ = clipToGrid(midPosZ, curPosZface);
                    vec3 midVelZ = getVelocity(clippedMidPosZ);
                    oldPosZface = curPosZface - dt * midVelZ;
                }

                vec3 clippedOldPosZ = clipToGrid(oldPosZface, curPosZface);
                vec3 newVelZ = getVelocity(clippedOldPosZ);
                target.mW(i, j, k) = newVelZ[2];
            }
        }

    }

    // Linghan 2018-04-10

    // Then save the result to our object
    mU = target.mU;
    mV = target.mV;
    mW = target.mW;
}

void MACGrid::advectTemperature(double dt)
{
    // TODO: Calculate new temp and store in target

    // TODO: Get rid of this line after you implement yours
    //target.mT = mT;

    // TODO: Your code is here. It builds target.mT for all cells.
    FOR_EACH_CELL {
        if(isInBox(i, j, k)) {
            target.mT(i, j, k) = 0;
            continue;
        }

        vec3 curPos = getCenter(i, j, k);
        vec3 curVel = getVelocity(curPos);

        vec3 oldPos;
        if(theBackTraceMode == FORWARDEULER) {
            // Forward Euler
            oldPos = curPos - dt * curVel;
        }

        else if(theBackTraceMode == RK2) {
            // RK2
            vec3 midPos = curPos - 0.5 * dt * curVel;
            vec3 clippedMidPos = clipToGrid(midPos, curPos);
            vec3 midVel = getVelocity(clippedMidPos);

            oldPos = curPos - dt * midVel;
        }

        vec3 clippedOldPos = clipToGrid(oldPos, curPos);
        double newTemp = getTemperature(clippedOldPos);

        target.mT(i, j, k) = newTemp;
    }
    // Linghan 2018-04-10

    // Then save the result to our object
    mT = target.mT;
}


void MACGrid::advectRenderingParticles(double dt) {
	rendering_particles_vel.resize(rendering_particles.size());
	for (size_t p = 0; p < rendering_particles.size(); p++) {
		vec3 currentPosition = rendering_particles[p];
        vec3 currentVelocity = getVelocity(currentPosition);
        vec3 nextPosition = currentPosition + currentVelocity * dt;
        vec3 clippedNextPosition = clipToGrid(nextPosition, currentPosition);
        // Keep going...
        vec3 nextVelocity = getVelocity(clippedNextPosition);
        vec3 averageVelocity = (currentVelocity + nextVelocity) / 2.0;
        vec3 betterNextPosition = currentPosition + averageVelocity * dt;
        vec3 clippedBetterNextPosition = clipToGrid(betterNextPosition, currentPosition);
        rendering_particles[p] = clippedBetterNextPosition;
		rendering_particles_vel[p] = averageVelocity;
	}
}

void MACGrid::advectDensity(double dt)
{
    // TODO: Calculate new densitities and store in target

    // TODO: Get rid of this line after you implement yours
    //target.mD = mD;

    // TODO: Your code is here. It builds target.mD for all cells.
	FOR_EACH_CELL {
        if(isInBox(i, j, k)) {
            target.mD(i, j, k) = 0;
            continue;
        }

		vec3 curPos = getCenter(i, j, k);
        vec3 curVel = getVelocity(curPos);

		vec3 oldPos;
		if(theBackTraceMode == FORWARDEULER) {
			// Forward Euler
			oldPos = curPos - dt * curVel;
		}

		else if(theBackTraceMode == RK2) {
			// RK2
			vec3 midPos = curPos - 0.5 * dt * curVel;
			vec3 clippedMidPos = clipToGrid(midPos, curPos);
			vec3 midVel = getVelocity(clippedMidPos);

			oldPos = curPos - dt * midVel;
		}

		vec3 clippedOldPos = clipToGrid(oldPos, curPos);
		double newDens = getDensity(clippedOldPos);

		target.mD(i, j, k) = newDens;
	}
	// Linghan 2018-04-10

    // Then save the result to our object
    mD = target.mD;

}

void MACGrid::computeBuoyancy(double dt)
{
	// TODO: Calculate buoyancy and store in target

    // TODO: Get rid of this line after you implement yours
    //target.mV = mV;

    // TODO: Your code is here. It modifies target.mV for all y face velocities.
    FOR_EACH_YFACE {
        if(j == 0 || j == theDim[MACGrid::Y] || isBoxBoundaryFace(MACGrid::Y, i, j, k)) target.mV(i, j, k) = 0;
        else {
            vec3 pos = getFacePosition(MACGrid::Y, i, j, k);
            double density = getDensity(pos);
            double temp = getTemperature(pos);
            double forceBuoy = - theBuoyancyAlpha * density + theBuoyancyBeta * (temp - theBuoyancyAmbientTemperature);
            target.mV(i, j, k) = mV(i, j, k) + forceBuoy;
        }
    }

    /*GridData forceBuoy; forceBuoy.initialize(0.0);

    FOR_EACH_CELL {
        forceBuoy(i, j, k) = - theBuoyancyAlpha * mD(i, j, k)
                    + theBuoyancyBeta * (mT(i, j, k) - theBuoyancyAmbientTemperature);
    }

    FOR_EACH_YFACE {
        if(j == 0 || j == theDim[MACGrid::Y]) target.mV(i, j, k) = 0;
        else {
            double increase = 0.5 * dt * (forceBuoy(i, j - 1, k) + forceBuoy(i, j, k));
            target.mV(i, j, k) = mV(i, j, k) + increase;
            //std::cout << target.mV(i, j, k) << " " << mV(i, j, k) << std::endl;
        }
    }
    // Linghan 2018-04-12 */

    // and then save the result to our object
    mV = target.mV;
}

void MACGrid::computeVorticityConfinement(double dt)
{
   // TODO: Calculate vorticity confinement forces

    // Apply the forces to the current velocity and store the result in target
	// STARTED.

    // TODO: Get rid of this line after you implement yours
	//target.mU = mU;
	//target.mV = mV;
	//target.mW = mW;

    // TODO: Your code is here. It modifies target.mU,mV,mW for all faces.
    GridData omegaX; omegaX.initialize(0.0);
    GridData omegaY; omegaY.initialize(0.0);
    GridData omegaZ; omegaZ.initialize(0.0);
    GridData omegaLength; omegaLength.initialize(0.0);

    // First, for every cell, compute omega vector, and then |omega|
    double twoSize = 2 * theCellSize;
    FOR_EACH_CELL {
        if(isInBox(i, j, k)) continue;

        double w_i_jplus1_k = getVelocityZ(getCenter(i, j+1, k));
        double w_i_jminus1_k = getVelocityZ(getCenter(i, j-1, k));
        double v_i_j_kplus1 = getVelocityY(getCenter(i, j, k+1));
        double v_i_j_kminus1 = getVelocityY(getCenter(i, j, k-1));
        omegaX(i, j, k) = ((w_i_jplus1_k - w_i_jminus1_k) - (v_i_j_kplus1 - v_i_j_kminus1)) / twoSize;

        double u_i_j_kplus1 = getVelocityX(getCenter(i, j, k+1));
        double u_i_j_kminus1 = getVelocityX(getCenter(i, j, k-1));
        double w_iplus1_j_k = getVelocityZ(getCenter(i+1, j, k));
        double w_iminus1_j_k = getVelocityZ(getCenter(i-1, j, k));
        omegaY(i, j, k) = ((u_i_j_kplus1 - u_i_j_kminus1) - (w_iplus1_j_k - w_iminus1_j_k)) / twoSize;

        double v_iplus1_j_k = getVelocityY(getCenter(i+1, j, k));
        double v_iminus1_j_k = getVelocityY(getCenter(i-1, j, k));
        double u_i_jplus1_k = getVelocityX(getCenter(i, j+1, k));
        double u_i_jminus1_k = getVelocityX(getCenter(i, j-1, k));
        omegaZ(i, j, k) = ((v_iplus1_j_k - v_iminus1_j_k) - (u_i_jplus1_k - u_i_jminus1_k)) / twoSize;

        vec3 omega(omegaX(i, j, k), omegaY(i, j, k), omegaZ(i, j, k));
        omegaLength(i, j, k) = omega.Length();
    }

    // Second, compute derivative|omega| for each cell respectively.
    // Finally, compute N for each cell and force

    GridData forceConfX; forceConfX.initialize(0.0);
    GridData forceConfY; forceConfY.initialize(0.0);
    GridData forceConfZ; forceConfZ.initialize(0.0);

    FOR_EACH_CELL {
        if(isInBox(i, j, k)) continue;

        double dOmegaX = (omegaLength(i+1, j, k) - omegaLength(i-1, j, k)) / twoSize;
        double dOmegaY = (omegaLength(i, j+1, k) - omegaLength(i, j-1, k)) / twoSize;
        double dOmegaZ = (omegaLength(i, j, k+1) - omegaLength(i, j, k-1)) / twoSize;

        vec3 dOmega(dOmegaX, dOmegaY, dOmegaZ);
        vec3 N = dOmega / (dOmega.Length() + 0.0000000001);
        vec3 omega(omegaX(i, j, k), omegaY(i, j, k), omegaZ(i, j, k));

        vec3 forceConf = theVorticityEpsilon * theCellSize * (N.Cross(omega));

        forceConfX(i, j, k) = forceConf[0];
        forceConfY(i, j, k) = forceConf[1];
        forceConfZ(i, j, k) = forceConf[2];
    }

    FOR_EACH_FACE {
        // X-Face
        if(isValidFace(MACGrid::X, i, j, k)) {
            if(i == 0 || i == theDim[MACGrid::X] || isBoxBoundaryFace(MACGrid::X, i, j, k)) target.mU(i, j, k) = 0;
            else {
                double increase = 0.5 * dt * (forceConfX(i - 1, j, k) + forceConfX(i, j, k));
                target.mU(i, j, k) = mU(i, j, k) + increase;
            }
        }

        // Y-Face
        if(isValidFace(MACGrid::Y, i, j, k)) {
            if(j == 0 || j == theDim[MACGrid::Y] || isBoxBoundaryFace(MACGrid::Y, i, j, k)) target.mV(i, j, k) = 0;
            else {
                double increase = 0.5 * dt * (forceConfY(i, j - 1, k) + forceConfY(i, j, k));
                target.mV(i, j, k) = mV(i, j, k) + increase;
            }
        }

        // Z-Face
        if(isValidFace(MACGrid::Z, i, j, k)) {
            if(k == 0 || k == theDim[MACGrid::Z] || isBoxBoundaryFace(MACGrid::Z, i, j, k)) target.mW(i, j, k) = 0;
            else {
                double increase = 0.5 * dt * (forceConfZ(i, j, k - 1) + forceConfZ(i, j, k));
                target.mW(i, j, k) = mW(i, j, k) + increase;
            }
        }
    }

    // Linghan 2018-04-12

    // Then save the result to our object
    mU = target.mU;
    mV = target.mV;
    mW = target.mW;
}

void MACGrid::computeWind() {

    FOR_EACH_FACE {
                // X-Face
         if (isValidFace(MACGrid::X, i, j, k)) {
             if (i == 0 || i == theDim[MACGrid::X]) target.mU(i, j, k) = 0;
             else {
                 if(j < 20) target.mU(i, j, k) = mU(i, j, k) + 1;
                 else if(j < 40) target.mU(i, j, k) = mU(i, j, k) - 1;
                 else target.mU(i, j, k) = mU(i, j, k) + 1;
             }
         }
    }

    mU = target.mU;
}

void MACGrid::addExternalForces(double dt)
{
   computeBuoyancy(dt);
   computeVorticityConfinement(dt);
   //computeWind();
}

void MACGrid::project(double dt)
{
   // TODO: Solve Ap = d for pressure
   // 1. Contruct d
   // 2. Construct A 
   // 3. Solve for p
   // Subtract pressure from our velocity and save in target
	// STARTED.

    // TODO: Get rid of these 3 lines after you implement yours
    //target.mU = mU;
	//target.mV = mV;
	//target.mW = mW;


    // TODO: Your code is here. It solves for a pressure field and modifies target.mU,mV,mW for all faces.
    // First, construct d, the entry of which is - (u_i+1,j,k - u_i,j,k + v_i,j+1,k - v_i,j,k + w_i,j,k+1 - w_i,j,k) * h * rho / dt
    // For boundary, if cell (i+1, j, k) is solid, then, + u(i+1, j, k) * h * rho / dt
    GridData d;
    d.initialize(0.0);

    double h_rho_by_dt = theCellSize * theAirDensity / dt;
    double dt_by_h_rho = 1 / h_rho_by_dt;

    FOR_EACH_CELL {
        if(isInBox(i, j, k)) continue;

        d(i, j, k) = (mU(i, j, k) - mU(i + 1, j, k)
                    + mV(i, j, k) - mV(i, j + 1, k)
                    + mW(i, j, k) - mW(i, j, k + 1)) * h_rho_by_dt;

        // Boundary Condition
        if(i == 0) {
            d(i, j, k) = d(i, j, k) + mU(i, j, k) * h_rho_by_dt;
        }
        if(i == theDim[MACGrid::X]) {
            d(i, j, k) = d(i, j, k) + mU(i + 1, j, k) * h_rho_by_dt;
        }
        if(j == 0) {
            d(i, j, k) = d(i, j, k) + mV(i, j, k) * h_rho_by_dt;
        }
        if(j == theDim[MACGrid::Y]) {
            d(i, j, k) = d(i, j, k) + mV(i, j + 1, k) * h_rho_by_dt;
        }
        if(k == 0) {
            d(i, j, k) = d(i, j, k) + mW(i, j, k) * h_rho_by_dt;
        }
        if(k == theDim[MACGrid::Z]) {
            d(i, j, k) = d(i, j, k) + mW(i, j, k + 1) * h_rho_by_dt;
        }
    }

    // Second, construct A, which is already computed using calculateAMatrix() function
    // Third, solve for p using preconditionedConjugateGradient() function
    if(useEigen)
        useEigenComputeCG(target.mP, d, 100, 0.000001);
    else
        preconditionedConjugateGradient(AMatrix, target.mP, d, 500, 0.000001);

    // Finally, subtract pressure from our velocity
    // u^(n+1)_i,j,k = u^_i,j,k - dt/(airDensity*h) * (P_i,j,k - P_i-1,j,k)
    //               = u^*_i,j,k - h * (mP_i,j,k - mP_i-1,j,k)
    FOR_EACH_FACE {
        if(isValidFace(MACGrid::X, i, j, k)) {
            if(i == 0 || i == theDim[MACGrid::X] || isBoxBoundaryFace(MACGrid::X, i, j, k)) target.mU(i, j, k) = 0;
            else target.mU(i, j, k) = mU(i, j, k) - dt_by_h_rho * (target.mP(i, j, k) - target.mP(i-1, j, k));
        }

        if(isValidFace(MACGrid::Y, i, j, k)) {
            if(j == 0 || j == theDim[MACGrid::Y] || isBoxBoundaryFace(MACGrid::Y, i, j, k)) target.mV(i, j, k) = 0;
            else target.mV(i, j, k) = mV(i, j, k) - dt_by_h_rho * (target.mP(i, j, k) - target.mP(i, j-1, k));

            //if(target.mV(i, j, k) != 0) PRINT_LINE(target.mV(i, j, k));
        }

        if(isValidFace(MACGrid::Z, i, j, k)) {
            if(k == 0 || k == theDim[MACGrid::Z] || isBoxBoundaryFace(MACGrid::Z, i, j, k)) target.mW(i, j, k) = 0;
            else target.mW(i, j, k) = mW(i, j, k) - dt_by_h_rho * (target.mP(i, j, k) - target.mP(i, j, k-1));
        }

    }

    // Linghan 2018-04-12

	#ifdef _DEBUG
	// Check border velocities:
	FOR_EACH_FACE {
		if (isValidFace(MACGrid::X, i, j, k)) {

			if (i == 0) {
				if (abs(target.mU(i,j,k)) > 0.0000001) {
					PRINT_LINE( "LOW X:  " << target.mU(i,j,k) );
					//target.mU(i,j,k) = 0;
				}
			}

			if (i == theDim[MACGrid::X]) {
				if (abs(target.mU(i,j,k)) > 0.0000001) {
					PRINT_LINE( "HIGH X: " << target.mU(i,j,k) );
					//target.mU(i,j,k) = 0;
				}
			}

		}
		if (isValidFace(MACGrid::Y, i, j, k)) {
			

			if (j == 0) {
				if (abs(target.mV(i,j,k)) > 0.0000001) {
					PRINT_LINE( "LOW Y:  " << target.mV(i,j,k) );
					//target.mV(i,j,k) = 0;
				}
			}

			if (j == theDim[MACGrid::Y]) {
				if (abs(target.mV(i,j,k)) > 0.0000001) {
					PRINT_LINE( "HIGH Y: " << target.mV(i,j,k) );
					//target.mV(i,j,k) = 0;
				}
			}

		}
		if (isValidFace(MACGrid::Z, i, j, k)) {
			
			if (k == 0) {
				if (abs(target.mW(i,j,k)) > 0.0000001) {
					PRINT_LINE( "LOW Z:  " << target.mW(i,j,k) );
					//target.mW(i,j,k) = 0;
				}
			}

			if (k == theDim[MACGrid::Z]) {
				if (abs(target.mW(i,j,k)) > 0.0000001) {
					PRINT_LINE( "HIGH Z: " << target.mW(i,j,k) );
					//target.mW(i,j,k) = 0;
				}
			}
		}
	}
	#endif


   // Then save the result to our object
   mP = target.mP;
   mU = target.mU;
   mV = target.mV;
   mW = target.mW;

    #ifdef _DEBUG
   // IMPLEMENT THIS AS A SANITY CHECK: assert (checkDivergence());
   // TODO: Fix duplicate code:
   FOR_EACH_CELL {
	   // Construct the vector of divergences d:
        double velLowX = mU(i,j,k);
        double velHighX = mU(i+1,j,k);
        double velLowY = mV(i,j,k);
        double velHighY = mV(i,j+1,k);
        double velLowZ = mW(i,j,k);
        double velHighZ = mW(i,j,k+1);
		double divergence = ((velHighX - velLowX) + (velHighY - velLowY) + (velHighZ - velLowZ)) / theCellSize;
		if (abs(divergence) > 0.02 ) {
			PRINT_LINE("WARNING: Divergent! ");
			PRINT_LINE("Divergence: " << divergence);
			PRINT_LINE("Cell: " << i << ", " << j << ", " << k);
		}
   }
    #endif


}

vec3 MACGrid::getVelocity(const vec3& pt)
{
   vec3 vel;
   vel[0] = getVelocityX(pt); 
   vel[1] = getVelocityY(pt); 
   vel[2] = getVelocityZ(pt); 
   return vel;
}

double MACGrid::getVelocityX(const vec3& pt)
{
   return mU.interpolate(pt);
}

double MACGrid::getVelocityY(const vec3& pt)
{
   return mV.interpolate(pt);
}

double MACGrid::getVelocityZ(const vec3& pt)
{
   return mW.interpolate(pt);
}

double MACGrid::getTemperature(const vec3& pt)
{
   return mT.interpolate(pt);
}

double MACGrid::getDensity(const vec3& pt)
{
   return mD.interpolate(pt);
}

vec3 MACGrid::getCenter(int i, int j, int k)
{
   double xstart = theCellSize/2.0;
   double ystart = theCellSize/2.0;
   double zstart = theCellSize/2.0;

   double x = xstart + i*theCellSize;
   double y = ystart + j*theCellSize;
   double z = zstart + k*theCellSize;
   return vec3(x, y, z);
}


vec3 MACGrid::getRewoundPosition(const vec3 & currentPosition, const double dt) {

	/*
	// EULER (RK1):
	vec3 currentVelocity = getVelocity(currentPosition);
	vec3 rewoundPosition = currentPosition - currentVelocity * dt;
	vec3 clippedRewoundPosition = clipToGrid(rewoundPosition, currentPosition);
	return clippedRewoundPosition;
	*/

	// HEUN / MODIFIED EULER (RK2):
	vec3 currentVelocity = getVelocity(currentPosition);
	vec3 rewoundPosition = currentPosition - currentVelocity * dt;
	vec3 clippedRewoundPosition = clipToGrid(rewoundPosition, currentPosition);
	// Keep going...
	vec3 rewoundVelocity = getVelocity(clippedRewoundPosition);
	vec3 averageVelocity = (currentVelocity + rewoundVelocity) / 2.0;
	vec3 betterRewoundPosition = currentPosition - averageVelocity * dt;
	vec3 clippedBetterRewoundPosition = clipToGrid(betterRewoundPosition, currentPosition);
	return clippedBetterRewoundPosition;

}


vec3 MACGrid:: clipToGrid(const vec3& outsidePoint, const vec3& insidePoint) {
	/*
	// OLD:
	vec3 rewindPosition = outsidePoint;
	if (rewindPosition[0] < 0) rewindPosition[0] = 0; // TEMP!
	if (rewindPosition[1] < 0) rewindPosition[1] = 0; // TEMP!
	if (rewindPosition[2] < 0) rewindPosition[2] = 0; // TEMP!
	if (rewindPosition[0] > theDim[MACGrid::X]) rewindPosition[0] = theDim[MACGrid::X]; // TEMP!
	if (rewindPosition[1] > theDim[MACGrid::Y]) rewindPosition[1] = theDim[MACGrid::Y]; // TEMP!
	if (rewindPosition[2] > theDim[MACGrid::Z]) rewindPosition[2] = theDim[MACGrid::Z]; // TEMP!
	return rewindPosition;
	*/

	vec3 clippedPoint = outsidePoint;

	for (int i = 0; i < 3; i++) {
		if (clippedPoint[i] < 0) {
			vec3 distance = clippedPoint - insidePoint;
			double newDistanceI = 0 - insidePoint[i];
			double ratio = newDistanceI / distance[i];
			clippedPoint = insidePoint + distance * ratio;
		}
		if (clippedPoint[i] > getSize(i)) {
			vec3 distance = clippedPoint - insidePoint;
			double newDistanceI = getSize(i) - insidePoint[i];
			double ratio = newDistanceI / distance[i];
			clippedPoint = insidePoint + distance * ratio;
		}

        // Linghan 2018-04-18. If point in box
        if (clippedPoint[0] > boxMinPos && clippedPoint[0] < boxMaxPos &&
            clippedPoint[1] > boxMinPos && clippedPoint[1] < boxMaxPos &&
            clippedPoint[2] > boxMinPos && clippedPoint[2] < boxMaxPos) {
            if (2 * clippedPoint[i] < (boxMaxPos + boxMinPos)) {
                vec3 distance = clippedPoint - insidePoint;
                double newDistanceI = boxMinPos - insidePoint[i];
                double ratio = newDistanceI / distance[i];
                clippedPoint = insidePoint + distance * ratio;
            }
            else {
                vec3 distance = clippedPoint - insidePoint;
                double newDistanceI = boxMaxPos - insidePoint[i];
                double ratio = newDistanceI / distance[i];
                clippedPoint = insidePoint + distance * ratio;
            }
        }
	}

#ifdef _DEBUG
	// Make sure the point is now in the grid:
	if (clippedPoint[0] < 0 || clippedPoint[1] < 0 || clippedPoint[2] < 0 || clippedPoint[0] > getSize(0) || clippedPoint[1] > getSize(1) || clippedPoint[2] > getSize(2)) {
		PRINT_LINE("WARNING: Clipped point is outside grid!");
	}
#endif

	return clippedPoint;

}


double MACGrid::getSize(int dimension) {
	return theDim[dimension] * theCellSize;
}


int MACGrid::getCellIndex(int i, int j, int k)
{
	return i + j * theDim[MACGrid::X] + k * theDim[MACGrid::Y] * theDim[MACGrid::X];
}


int MACGrid::getNumberOfCells()
{
	return theDim[MACGrid::X] * theDim[MACGrid::Y] * theDim[MACGrid::Z];
}


bool MACGrid::isValidCell(int i, int j, int k)
{
	if (i >= theDim[MACGrid::X] || j >= theDim[MACGrid::Y] || k >= theDim[MACGrid::Z]) {
		return false;
	}

	if (i < 0 || j < 0 || k < 0) {
		return false;
	}

    // Linghan 2018-04-18
    if (isInBox(i, j, k)) {
        return false;
    }

	return true;
}


bool MACGrid::isValidFace(int dimension, int i, int j, int k)
{
	if (dimension == 0) {
		if (i > theDim[MACGrid::X] || j >= theDim[MACGrid::Y] || k >= theDim[MACGrid::Z]) {
			return false;
		}
        // Linghan 2018-04-18
        if (i > boxMin && i <= boxMax && j >= boxMin && j <= boxMax && k >= boxMin && k <= boxMax)
            return false;

	} else if (dimension == 1) {
		if (i >= theDim[MACGrid::X] || j > theDim[MACGrid::Y] || k >= theDim[MACGrid::Z]) {
			return false;
		}
        // Linghan 2018-04-18
        if (i >= boxMin && i <= boxMax && j > boxMin && j <= boxMax && k >= boxMin && k <= boxMax)
            return false;

	} else if (dimension == 2) {
		if (i >= theDim[MACGrid::X] || j >= theDim[MACGrid::Y] || k > theDim[MACGrid::Z]) {
			return false;
		}

        // Linghan 2018-04-18
        if (i >= boxMin && i <= boxMax && j >= boxMin && j <= boxMax && k > boxMin && k <= boxMax)
            return false;
	}

	if (i < 0 || j < 0 || k < 0) {
		return false;
	}

	return true;
}

// Linghan 2018-04-18
bool MACGrid::isInBox(int i, int j, int k)
{
    if (i < boxMin) return false;
    if (i > boxMax) return false;
    if (j < boxMin) return false;
    if (j > boxMax) return false;
    if (k < boxMin) return false;
    if (k > boxMax) return false;
    return true;
}

// Linghan 2018-04-18
bool MACGrid::isBoxBoundaryFace(int dimension, int i, int j, int k)
{
    if (dimension == 0) {
        if ((i == boxMin || i == boxMax+1) && j >= boxMin && j <= boxMax && k >= boxMin && k <= boxMax)
            return true;

    } else if (dimension == 1) {
        if (i >= boxMin && i <= boxMax && (j == boxMin || j == boxMax+1) && k >= boxMin && k <= boxMax)
            return true;

    } else if (dimension == 2) {
        if (i >= boxMin && i <= boxMax && j >= boxMin && j <= boxMax && (k == boxMin || k == boxMax+1))
            return true;
    }

    return false;
}

vec3 MACGrid::getFacePosition(int dimension, int i, int j, int k)
{
	if (dimension == 0) {
		return vec3(i * theCellSize, (j + 0.5) * theCellSize, (k + 0.5) * theCellSize);
	} else if (dimension == 1) {
		return vec3((i + 0.5) * theCellSize, j * theCellSize, (k + 0.5) * theCellSize);
	} else if (dimension == 2) {
		return vec3((i + 0.5) * theCellSize, (j + 0.5) * theCellSize, k * theCellSize);
	}

	return vec3(0,0,0); //???

}

void MACGrid::calculateAMatrix() {

    // coefficients: self -> number of fluid neighbors;
    //      fluid neighbor -> -1; others -> 0
	FOR_EACH_CELL {
        // Linghan 2018-04-18
        if(isInBox(i, j, k)) {
            AMatrix.diag(i,j,k) = 1;
            continue;
        }

		int numFluidNeighbors = 0;
		if (i-1 >= 0 && !isInBox(i-1, j, k)) {
			AMatrix.plusI(i-1,j,k) = -1;
			numFluidNeighbors++;
		}
		if (i+1 < theDim[MACGrid::X] && !isInBox(i+1, j, k)) {
			AMatrix.plusI(i,j,k) = -1;
			numFluidNeighbors++;
		}
		if (j-1 >= 0 && !isInBox(i, j-1, k)) {
			AMatrix.plusJ(i,j-1,k) = -1;
			numFluidNeighbors++;
		}
		if (j+1 < theDim[MACGrid::Y] && !isInBox(i, j+1, k)) {
			AMatrix.plusJ(i,j,k) = -1;
			numFluidNeighbors++;
		}
		if (k-1 >= 0 && !isInBox(i, j, k-1)) {
			AMatrix.plusK(i,j,k-1) = -1;
			numFluidNeighbors++;
		}
		if (k+1 < theDim[MACGrid::Z] && !isInBox(i, j, k+1)) {
			AMatrix.plusK(i,j,k) = -1;
			numFluidNeighbors++;
		}

		// Set the diagonal:
		AMatrix.diag(i,j,k) = numFluidNeighbors;
	}

}

bool MACGrid::preconditionedConjugateGradient(const GridDataMatrix & A, GridData & p, const GridData & d, int maxIterations, double tolerance) {
	// Solves Ap = d for p.

	FOR_EACH_CELL {
		p(i,j,k) = 0.0; // Initial guess p = 0.	
	}

	GridData r = d; // Residual vector.

    /*
	PRINT_LINE("r: ");
	FOR_EACH_CELL {
		PRINT_LINE(r(i,j,k));
	}
    */

	GridData z; z.initialize();
	applyPreconditioner(r, A, z); // Auxillary vector.
    /*
	PRINT_LINE("z: ");
	FOR_EACH_CELL {
		PRINT_LINE(z(i,j,k));
	}
    */

	GridData s = z; // Search vector;

	double sigma = dotProduct(z, r);

	for (int iteration = 0; iteration < maxIterations; iteration++) {

		double rho = sigma; // According to TA. Here???

		apply(A, s, z); // z = applyA(s);

		double alpha = rho/dotProduct(z, s);

		GridData alphaTimesS; alphaTimesS.initialize();
		multiply(alpha, s, alphaTimesS);
		add(p, alphaTimesS, p);
		//p += alpha * s;

		GridData alphaTimesZ; alphaTimesZ.initialize();
		multiply(alpha, z, alphaTimesZ);
		subtract(r, alphaTimesZ, r);
		//r -= alpha * z;

		if (maxMagnitude(r) <= tolerance) {
			PRINT_LINE("PCG converged in " << (iteration + 1) << " iterations.");
            //PRINT_LINE("r: ");
            //FOR_EACH_CELL {
            //            PRINT_LINE(r(i,j,k)); }
			return true; //return p;
		}

		applyPreconditioner(r, A, z); // z = applyPreconditioner(r);

		double sigmaNew = dotProduct(z, r);

		double beta = sigmaNew / rho;

		GridData betaTimesS; betaTimesS.initialize();
		multiply(beta, s, betaTimesS);
		add(z, betaTimesS, s);
		//s = z + beta * s;

		sigma = sigmaNew;
	}

	PRINT_LINE( "PCG didn't converge!" );
	return false;

}


void MACGrid::calculatePreconditioner(const GridDataMatrix & A) {

	precon.initialize();

    double tao = 0.97;

    // TODO: Build the modified incomplete Cholesky preconditioner following Fig 4.2 on page 36 of Bridson's 2007 SIGGRAPH fluid course notes.
    //       This corresponds to filling in precon(i,j,k) for all cells
    FOR_EACH_CELL {
        if(!isInBox(i, j, k)) { // Linghan 2018-04-19
            double e = A.diag(i, j, k) - pow((A.plusI(i - 1, j, k) * precon(i - 1, j, k)), 2)
                       - pow((A.plusJ(i, j - 1, k) * precon(i, j - 1, k)), 2)
                       - pow((A.plusK(i, j, k - 1) * precon(i, j, k - 1)), 2)
                       - tao * (A.plusI(i - 1, j, k) * (A.plusJ(i - 1, j, k) + A.plusK(i - 1, j, k)) *
                                pow(precon(i - 1, j, k), 2)
                                + A.plusJ(i, j - 1, k) * (A.plusI(i, j - 1, k) + A.plusK(i, j - 1, k)) *
                                  pow(precon(i, j - 1, k), 2)
                                + A.plusK(i, j, k - 1) * (A.plusI(i, j, k - 1) + A.plusJ(i, j, k - 1)) *
                                  pow(precon(i, j, k - 1), 2));
            precon(i, j, k) = 1 / sqrt(e + pow(10, -30));
        }
    }

    // Linghan 2018-04-12

}


void MACGrid::applyPreconditioner(const GridData & r, const GridDataMatrix & A, GridData & z) {

    // TODO: change if(0) to if(1) after you implement calculatePreconditoner function.

    if(0) {

        // APPLY THE PRECONDITIONER:
        // Solve Lq = r for q:
        GridData q;
        q.initialize();
        FOR_EACH_CELL {
                    //if (A.diag(i,j,k) != 0.0) { // If cell is a fluid.
                    if(!isInBox(i, j, k)) { // Linghan 2018-04-19
                        double t = r(i, j, k) - A.plusI(i - 1, j, k) * precon(i - 1, j, k) * q(i - 1, j, k)
                                   - A.plusJ(i, j - 1, k) * precon(i, j - 1, k) * q(i, j - 1, k)
                                   - A.plusK(i, j, k - 1) * precon(i, j, k - 1) * q(i, j, k - 1);
                        q(i, j, k) = t * precon(i, j, k);
                    }
                    //}
                }
        // Solve L^Tz = q for z:
        FOR_EACH_CELL_REVERSE {
                    //if (A.diag(i,j,k) != 0.0) { // If cell is a fluid.
                    if(!isInBox(i, j, k)) { // Linghan 2018-04-19
                        double t = q(i, j, k) - A.plusI(i, j, k) * precon(i, j, k) * z(i + 1, j, k)
                                   - A.plusJ(i, j, k) * precon(i, j, k) * z(i, j + 1, k)
                                   - A.plusK(i, j, k) * precon(i, j, k) * z(i, j, k + 1);
                        z(i, j, k) = t * precon(i, j, k);
                    }
                    //}
                }
    }
    else{
        // Unpreconditioned CG: Bypass preconditioner:
        z = r;
        return;
    }

}



double MACGrid::dotProduct(const GridData & vector1, const GridData & vector2) {
	
	double result = 0.0;

	FOR_EACH_CELL {
		result += vector1(i,j,k) * vector2(i,j,k);
	}

	return result;
}


void MACGrid::add(const GridData & vector1, const GridData & vector2, GridData & result) {
	
	FOR_EACH_CELL {
		result(i,j,k) = vector1(i,j,k) + vector2(i,j,k);
	}

}


void MACGrid::subtract(const GridData & vector1, const GridData & vector2, GridData & result) {
	
	FOR_EACH_CELL {
		result(i,j,k) = vector1(i,j,k) - vector2(i,j,k);
	}

}


void MACGrid::multiply(const double scalar, const GridData & vector, GridData & result) {
	
	FOR_EACH_CELL {
		result(i,j,k) = scalar * vector(i,j,k);
	}

}


double MACGrid::maxMagnitude(const GridData & vector) {
	
	double result = 0.0;

	FOR_EACH_CELL {
		if (abs(vector(i,j,k)) > result) result = abs(vector(i,j,k));
	}

	return result;
}


void MACGrid::apply(const GridDataMatrix & matrix, const GridData & vector, GridData & result) {
	
	FOR_EACH_CELL { // For each row of the matrix.

		double diag = 0;
		double plusI = 0;
		double plusJ = 0;
		double plusK = 0;
		double minusI = 0;
		double minusJ = 0;
		double minusK = 0;

		diag = matrix.diag(i,j,k) * vector(i,j,k);
		if (isValidCell(i+1,j,k)) plusI = matrix.plusI(i,j,k) * vector(i+1,j,k);
		if (isValidCell(i,j+1,k)) plusJ = matrix.plusJ(i,j,k) * vector(i,j+1,k);
		if (isValidCell(i,j,k+1)) plusK = matrix.plusK(i,j,k) * vector(i,j,k+1);
		if (isValidCell(i-1,j,k)) minusI = matrix.plusI(i-1,j,k) * vector(i-1,j,k);
		if (isValidCell(i,j-1,k)) minusJ = matrix.plusJ(i,j-1,k) * vector(i,j-1,k);
		if (isValidCell(i,j,k-1)) minusK = matrix.plusK(i,j,k-1) * vector(i,j,k-1);

		result(i,j,k) = diag + plusI + plusJ + plusK + minusI + minusJ + minusK;
	}

}

void MACGrid::saveSmoke(const char* fileName) {
	std::ofstream fileOut(fileName);
	if (fileOut.is_open()) {
		FOR_EACH_CELL {
			fileOut << mD(i,j,k) << std::endl;
		}
		fileOut.close();
	}
}

void MACGrid::saveParticle(std::string filename){
	Partio::ParticlesDataMutable *parts = Partio::create();
	Partio::ParticleAttribute posH, vH;
	posH = parts->addAttribute("position", Partio::VECTOR, 3);
	vH = parts->addAttribute("v", Partio::VECTOR, 3);
	for (unsigned int i = 0; i < rendering_particles.size(); i++)
	{
		int idx = parts->addParticle();
		float *p = parts->dataWrite<float>(posH, idx);
		float *v = parts->dataWrite<float>(vH, idx);
		for (int k = 0; k < 3; k++)
		{
			p[k] = rendering_particles[i][k];
			v[k] = rendering_particles_vel[i][k];
		}
	}
	
	Partio::write(filename.c_str(), *parts);
	parts->release();
}

void MACGrid::saveDensity(std::string filename){
	Partio::ParticlesDataMutable *density_field = Partio::create();
	Partio::ParticleAttribute posH, rhoH;
	posH = density_field->addAttribute("position", Partio::VECTOR, 3);
	rhoH = density_field->addAttribute("density", Partio::VECTOR, 1);
	FOR_EACH_CELL{
		int idx = density_field->addParticle();
		float *p = density_field->dataWrite<float>(posH, idx);
		float *rho = density_field->dataWrite<float>(rhoH, idx);
		vec3 cellCenter = getCenter(i, j, k);
		for (int l = 0; l < 3; l++)
		{
			p[l] = cellCenter[l];
		}
		rho[0] = getDensity(cellCenter);
	}
	Partio::write(filename.c_str(), *density_field);
	density_field->release();
}

void MACGrid::draw(const Camera& c)
{   
   drawWireGrid();
   if (theDisplayVel) drawVelocities();   
   if (theRenderMode == CUBES) drawSmokeCubes(c);
   else drawSmoke(c);
}

void MACGrid::drawVelocities()
{
   // draw line at each center
   //glColor4f(0.0, 1.0, 0.0, 1.0);
   glBegin(GL_LINES);
      FOR_EACH_CELL
      {
         vec3 pos = getCenter(i,j,k);
         vec3 vel = getVelocity(pos);
         if (vel.Length() > 0.0001)
         {
           //vel.Normalize(); 
           vel *= theCellSize/2.0;
           vel += pos;
		   glColor4f(1.0, 1.0, 0.0, 1.0);
           glVertex3dv(pos.n);
		   glColor4f(0.0, 1.0, 0.0, 1.0);
           glVertex3dv(vel.n);
         }
      }
   glEnd();
}

vec4 MACGrid::getRenderColor(int i, int j, int k)
{
	
	double value = mD(i, j, k); 
	vec4 coldColor(0.5, 0.5, 1.0, value);
	vec4 hotColor(1.0, 0.5, 0.5, value);
    return LERP(coldColor, hotColor, mT(i, j, k));
	

	/*
	// OLD:
    double value = mD(i, j, k); 
    return vec4(1.0, 0.9, 1.0, value);
	*/
}

vec4 MACGrid::getRenderColor(const vec3& pt)
{
	double value = getDensity(pt);
	vec4 coldColor(0.5, 0.5, 1.0, value);
	vec4 hotColor(1.0, 0.5, 0.5, value);
    return LERP(coldColor, hotColor, getTemperature(pt));

	/*
	// OLD:
    double value = getDensity(pt); 
    return vec4(1.0, 1.0, 1.0, value);
	*/
}

void MACGrid::drawZSheets(bool backToFront)
{
   // Draw K Sheets from back to front
   double back =  (theDim[2])*theCellSize;
   double top  =  (theDim[1])*theCellSize;
   double right = (theDim[0])*theCellSize;
  
   double stepsize = theCellSize*0.25;

   double startk = back - stepsize;
   double endk = 0;
   double stepk = -theCellSize;

   if (!backToFront)
   {
      startk = 0;
      endk = back;   
      stepk = theCellSize;
   }

   for (double k = startk; backToFront? k > endk : k < endk; k += stepk)
   {
     for (double j = 0.0; j < top; )
      {
         glBegin(GL_QUAD_STRIP);
         for (double i = 0.0; i <= right; i += stepsize)
         {
            vec3 pos1 = vec3(i,j,k); 
            vec3 pos2 = vec3(i, j+stepsize, k); 

            vec4 color1 = getRenderColor(pos1);
            vec4 color2 = getRenderColor(pos2);

            glColor4dv(color1.n);
            glVertex3dv(pos1.n);

            glColor4dv(color2.n);
            glVertex3dv(pos2.n);
         } 
         glEnd();
         j+=stepsize;

         glBegin(GL_QUAD_STRIP);
         for (double i = right; i >= 0.0; i -= stepsize)
         {
            vec3 pos1 = vec3(i,j,k); 
            vec3 pos2 = vec3(i, j+stepsize, k); 

            vec4 color1 = getRenderColor(pos1);
            vec4 color2 = getRenderColor(pos2);

            glColor4dv(color1.n);
            glVertex3dv(pos1.n);

            glColor4dv(color2.n);
            glVertex3dv(pos2.n);
         } 
         glEnd();
         j+=stepsize;
      }
   }
}

void MACGrid::drawXSheets(bool backToFront)
{
   // Draw K Sheets from back to front
   double back =  (theDim[2])*theCellSize;
   double top  =  (theDim[1])*theCellSize;
   double right = (theDim[0])*theCellSize;
  
   double stepsize = theCellSize*0.25;

   double starti = right - stepsize;
   double endi = 0;
   double stepi = -theCellSize;

   if (!backToFront)
   {
      starti = 0;
      endi = right;   
      stepi = theCellSize;
   }

   for (double i = starti; backToFront? i > endi : i < endi; i += stepi)
   {
     for (double j = 0.0; j < top; )
      {
         glBegin(GL_QUAD_STRIP);
         for (double k = 0.0; k <= back; k += stepsize)
         {
            vec3 pos1 = vec3(i,j,k); 
            vec3 pos2 = vec3(i, j+stepsize, k); 

            vec4 color1 = getRenderColor(pos1);
            vec4 color2 = getRenderColor(pos2);

            glColor4dv(color1.n);
            glVertex3dv(pos1.n);

            glColor4dv(color2.n);
            glVertex3dv(pos2.n);
         } 
         glEnd();
         j+=stepsize;

         glBegin(GL_QUAD_STRIP);
         for (double k = back; k >= 0.0; k -= stepsize)
         {
            vec3 pos1 = vec3(i,j,k); 
            vec3 pos2 = vec3(i, j+stepsize, k); 

            vec4 color1 = getRenderColor(pos1);
            vec4 color2 = getRenderColor(pos2);

            glColor4dv(color1.n);
            glVertex3dv(pos1.n);

            glColor4dv(color2.n);
            glVertex3dv(pos2.n);
         } 
         glEnd();
         j+=stepsize;
      }
   }
}


void MACGrid::drawSmoke(const Camera& c)
{
   vec3 eyeDir = c.getBackward();
   double zresult = fabs(Dot(eyeDir, vec3(1,0,0)));
   double xresult = fabs(Dot(eyeDir, vec3(0,0,1)));
   //double yresult = fabs(Dot(eyeDir, vec3(0,1,0)));

   if (zresult < xresult)
   {      
      drawZSheets(c.getPosition()[2] < 0);
   }
   else 
   {
      drawXSheets(c.getPosition()[0] < 0);
   }
}

void MACGrid::drawSmokeCubes(const Camera& c)
{
   std::multimap<double, MACGrid::Cube, std::greater<double> > cubes;
   FOR_EACH_CELL
   {
      MACGrid::Cube cube;
      cube.color = getRenderColor(i,j,k);
      cube.pos = getCenter(i,j,k);
      cube.dist = DistanceSqr(cube.pos, c.getPosition());
      cubes.insert(make_pair(cube.dist, cube));
   } 

   // Draw cubes from back to front
   std::multimap<double, MACGrid::Cube, std::greater<double> >::const_iterator it;
   for (it = cubes.begin(); it != cubes.end(); ++it)
   {
      drawCube(it->second);
   }
}

void MACGrid::drawWireGrid()
{
   // Display grid in light grey, draw top & bottom

   double xstart = 0.0;
   double ystart = 0.0;
   double zstart = 0.0;
   double xend = theDim[0]*theCellSize;
   double yend = theDim[1]*theCellSize;
   double zend = theDim[2]*theCellSize;

   glPushAttrib(GL_LIGHTING_BIT | GL_LINE_BIT);
      glDisable(GL_LIGHTING);
      glColor3f(0.25, 0.25, 0.25);

      glBegin(GL_LINES);
      for (int i = 0; i <= theDim[0]; i++)
      {
         double x = xstart + i*theCellSize;
         glVertex3d(x, ystart, zstart);
         glVertex3d(x, ystart, zend);

         glVertex3d(x, yend, zstart);
         glVertex3d(x, yend, zend);
      }

      for (int i = 0; i <= theDim[2]; i++)
      {
         double z = zstart + i*theCellSize;
         glVertex3d(xstart, ystart, z);
         glVertex3d(xend, ystart, z);

         glVertex3d(xstart, yend, z);
         glVertex3d(xend, yend, z);
      }

      glVertex3d(xstart, ystart, zstart);
      glVertex3d(xstart, yend, zstart);

      glVertex3d(xend, ystart, zstart);
      glVertex3d(xend, yend, zstart);

      glVertex3d(xstart, ystart, zend);
      glVertex3d(xstart, yend, zend);

      glVertex3d(xend, ystart, zend);
      glVertex3d(xend, yend, zend);
      glEnd();
   glPopAttrib();

   glEnd();
}

#define LEN 0.5
void MACGrid::drawFace(const MACGrid::Cube& cube)
{
   glColor4dv(cube.color.n);
   glPushMatrix();
      glTranslated(cube.pos[0], cube.pos[1], cube.pos[2]);      
      glScaled(theCellSize, theCellSize, theCellSize);
      glBegin(GL_QUADS);
         glNormal3d( 0.0,  0.0, 1.0);
         glVertex3d(-LEN, -LEN, LEN);
         glVertex3d(-LEN,  LEN, LEN);
         glVertex3d( LEN,  LEN, LEN);
         glVertex3d( LEN, -LEN, LEN);
      glEnd();
   glPopMatrix();
}

void MACGrid::drawCube(const MACGrid::Cube& cube)
{
   glColor4dv(cube.color.n);
   glPushMatrix();
      glTranslated(cube.pos[0], cube.pos[1], cube.pos[2]);      
      glScaled(theCellSize, theCellSize, theCellSize);
      glBegin(GL_QUADS);
         glNormal3d( 0.0, -1.0,  0.0);
         glVertex3d(-LEN, -LEN, -LEN);
         glVertex3d(-LEN, -LEN,  LEN);
         glVertex3d( LEN, -LEN,  LEN);
         glVertex3d( LEN, -LEN, -LEN);         

         glNormal3d( 0.0,  0.0, -0.0);
         glVertex3d(-LEN, -LEN, -LEN);
         glVertex3d(-LEN,  LEN, -LEN);
         glVertex3d( LEN,  LEN, -LEN);
         glVertex3d( LEN, -LEN, -LEN);

         glNormal3d(-1.0,  0.0,  0.0);
         glVertex3d(-LEN, -LEN, -LEN);
         glVertex3d(-LEN, -LEN,  LEN);
         glVertex3d(-LEN,  LEN,  LEN);
         glVertex3d(-LEN,  LEN, -LEN);

         glNormal3d( 0.0, 1.0,  0.0);
         glVertex3d(-LEN, LEN, -LEN);
         glVertex3d(-LEN, LEN,  LEN);
         glVertex3d( LEN, LEN,  LEN);
         glVertex3d( LEN, LEN, -LEN);

         glNormal3d( 0.0,  0.0, 1.0);
         glVertex3d(-LEN, -LEN, LEN);
         glVertex3d(-LEN,  LEN, LEN);
         glVertex3d( LEN,  LEN, LEN);
         glVertex3d( LEN, -LEN, LEN);

         glNormal3d(1.0,  0.0,  0.0);
         glVertex3d(LEN, -LEN, -LEN);
         glVertex3d(LEN, -LEN,  LEN);
         glVertex3d(LEN,  LEN,  LEN);
         glVertex3d(LEN,  LEN, -LEN);
      glEnd();
   glPopMatrix();
}

// Linghan 2018-04-19
void MACGrid::calculateEigenAMatrix()
{
    index_map.clear();
    int n = theDim[0] * theDim[1] * theDim[2] - pow((boxMax - boxMin + 1), 3);
    AEigen = Eigen::SparseMatrix<double>(n, n);

    // map (i, j, k) to index
    int index = 0;
    FOR_EACH_CELL {
        if(!isInBox(i, j, k)) {
            index_map.insert(make_pair(std::vector<int>{i, j, k}, index));
            index++;
        }
    }

    FOR_EACH_CELL {
        if(!isInBox(i, j, k)) {
            int selfIdx = index_map.at(std::vector<int>{i, j, k});
            int numFluidNeighbors = 0;
            if (i-1 >= 0 && !isInBox(i-1, j, k)) {
                AEigen.insert(selfIdx, index_map.at(std::vector<int>{i-1, j, k})) = -1;
                numFluidNeighbors++;
            }
            if (i+1 < theDim[MACGrid::X] && !isInBox(i+1, j, k)) {
                AEigen.insert(selfIdx, index_map.at(std::vector<int>{i+1, j, k})) = -1;
                numFluidNeighbors++;
            }
            if (j-1 >= 0 && !isInBox(i, j-1, k)) {
                AEigen.insert(selfIdx, index_map.at(std::vector<int>{i, j-1, k})) = -1;
                numFluidNeighbors++;
            }
            if (j+1 < theDim[MACGrid::Y] && !isInBox(i, j+1, k)) {
                AEigen.insert(selfIdx, index_map.at(std::vector<int>{i, j+1, k})) = -1;
                numFluidNeighbors++;
            }
            if (k-1 >= 0 && !isInBox(i, j, k-1)) {
                AEigen.insert(selfIdx, index_map.at(std::vector<int>{i, j, k-1})) = -1;
                numFluidNeighbors++;
            }
            if (k+1 < theDim[MACGrid::Z] && !isInBox(i, j, k+1)) {
                AEigen.insert(selfIdx, index_map.at(std::vector<int>{i, j, k+1})) = -1;
                numFluidNeighbors++;
            }

            // Set the diagonal:
            AEigen.insert(selfIdx, selfIdx) = numFluidNeighbors;
        }
    }
    //std::cout << AEigen << std::endl;
    std::cout << "finish compute AEigen" << std::endl;
}

void MACGrid::useEigenComputeCG(GridData & p, const GridData & d, int maxIterations, double tolerance)
{
    int n = theDim[0] * theDim[1] * theDim[2] - pow((boxMax - boxMin + 1), 3);

    Eigen::VectorXd vecp(n), vecd(n);

    // fill in d
    FOR_EACH_CELL {
        if(!isInBox(i, j, k)) {
            vecd(index_map.at(std::vector<int>{i, j, k})) = d(i, j, k);
        }
    }
    //std::cout << vecd << std::endl;

    // compute
    Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower|Eigen::Upper, Eigen::IncompleteCholesky<double>> pcg;

    pcg.compute(AEigen);
    vecp = pcg.solve(vecd);

    //std::cout << "#iterations:     " << pcg.iterations() << std::endl;
    //std::cout << "estimated error: " << pcg.error()      << std::endl;

    // fill in p
    FOR_EACH_CELL {
        if(!isInBox(i, j, k)) {
            p(i, j, k) = vecp(index_map.at(std::vector<int>{i, j, k}));
        }
        else p(i, j, k) = 0;
    }
}

void MACGrid::updateBox()
{
    if(boxUp) {
        if(boxMax < theDim[0] - 1) {
            boxMin += 1;
            boxMax += 1;
        }
        else boxUp = false;
    }

    else {
        if(boxMin > 0) {
            boxMin -= 1;
            boxMax -= 1;
        }
        else boxUp = true;
    }

    boxMinPos = boxMin * theCellSize;
    boxMaxPos = (boxMax + 1) * theCellSize;

    if(useEigen)
        calculateEigenAMatrix();
    else {
        calculateAMatrix();
        calculatePreconditioner(AMatrix);
    }
}