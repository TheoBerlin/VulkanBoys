#define M_PI 3.1415926535897932384626433832795f

// Returns ±1
vec2 signNotZero(vec2 v) 
{
	return vec2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}

// Assume normalized input. Output is on [-1, 1] for each component.
vec2 float32x3_to_oct(in vec3 v)
{
	// Project the sphere onto the octahedron, and then onto the xy plane
	vec2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
	// Reflect the folds of the lower hemisphere over the diagonals
	return (v.z <= 0.0) ? ((1.0 - abs(p.yx)) * signNotZero(p)) : p;
}

vec3 oct_to_float32x3(vec2 e)
{
	vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
	if (v.z < 0) v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
	return normalize(v);
}

int findMSB(int x)
{
	int i;
	int mask;
	int res = -1;

	if (x < 0) 
	{
		x = ~x;
	}

	for(i = 0; i < 32; i++) 
	{
		mask = 0x80000000 >> i;
		if ((x & mask) != 0) 
		{
			res = 31 - i;
			break;
		}
	}

	return res;
}

vec3 probeStartPosition(in int numProbesPerDimension, in vec3 probeStep)
{
	return -probeStep * vec3(numProbesPerDimension - 1) / 2.0f; //This may be wrong
}

int gridCoordToProbeIndex(in vec3 gridCoords, in int numProbesPerDimension) 
{
    return int(gridCoords.x + numProbesPerDimension * (gridCoords.y + numProbesPerDimension * gridCoords.z));
}

ivec3 baseGridCoord(in vec3 X, in int numProbesPerDimension, in vec3 probeStep) 
{
    return clamp(ivec3((X - probeStartPosition(numProbesPerDimension, probeStep)) / probeStep), 
                ivec3(0, 0, 0), 
                ivec3(numProbesPerDimension) - ivec3(1, 1, 1));
}

/** Returns the index of the probe at the floor along each dimension. */
int baseProbeIndex(in vec3 X, in int numProbesPerDimension, in vec3 probeStep) 
{
    return gridCoordToProbeIndex(baseGridCoord(X, numProbesPerDimension, probeStep), numProbesPerDimension);
}

ivec3 probeIndexToGridCoord(in int probeIndex, in int numProbesPerDimension) 
{    
    /* Works for any # of probes */
    /*
    iPos.x = index % L.probeCounts.x;
    iPos.y = (index % (L.probeCounts.x * L.probeCounts.y)) / L.probeCounts.x;
    iPos.z = index / (L.probeCounts.x * L.probeCounts.y);
    */

    // Assumes probeCounts are powers of two.
    // Saves ~10ms compared to the divisions above
    // Precomputing the MSB actually slows this code down substantially
    ivec3 iPos;
    iPos.x = probeIndex & (numProbesPerDimension - 1);
    iPos.y = (probeIndex & ((numProbesPerDimension * numProbesPerDimension) - 1)) >> findMSB(numProbesPerDimension);
    iPos.z = probeIndex >> findMSB(numProbesPerDimension * numProbesPerDimension);

    return iPos;
}

/** gridCoords Coordinates of the probe, computed as part of the process. */
int nearestProbeIndex(in vec3 X, in int numProbesPerDimension, in vec3 probeStep, out vec3 gridCoords) 
{
    gridCoords = clamp(round((X - probeStartPosition(numProbesPerDimension, probeStep)) / probeStep),
                vec3(0, 0, 0), 
                vec3(numProbesPerDimension) - vec3(1, 1, 1));
    return gridCoordToProbeIndex(gridCoords, numProbesPerDimension);
}

/** 
    \param neighbors The 8 probes surrounding X
    \return Index into the neighbors array of the index of the nearest probe to X 
*/
int nearestProbeIndices(in vec3 X, in int numProbesPerDimension, in vec3 probeStep) 
{
    vec3 maxProbeCoords = ivec3(numProbesPerDimension) - ivec3(1, 1, 1);
    vec3 floatProbeCoords = (X - probeStartPosition(numProbesPerDimension, probeStep)) / probeStep; //This may be wrong
    vec3 baseProbeCoords = clamp(floor(floatProbeCoords), ivec3(0, 0, 0), maxProbeCoords);

    float minDist = 10.0f;
    int nearestIndex = -1;

    for (int i = 0; i < 8; ++i) 
	{
        vec3 newProbeCoords = min(baseProbeCoords + vec3(i & 1, (i >> 1) & 1, (i >> 2) & 1), maxProbeCoords);
        float d = length(newProbeCoords - floatProbeCoords);

        if (d < minDist) 
		{
            minDist = d;
            nearestIndex = i;
        }
       
    }

    return nearestIndex;
}

vec3 gridCoordToPosition(in vec3 c, in int numProbesPerDimension, in vec3 probeStep) 
{
    return probeStep * c + probeStartPosition(numProbesPerDimension, probeStep);
}

/** GLSL's dot on ivec3 returns a float. This is an all-integer version */
int idot(ivec3 a, ivec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/**
   \param baseProbeIndex Index into L.radianceProbeGrid's TEXTURE_2D_ARRAY. This is the probe
   at the floor of the current ray sampling position.

   \param relativeIndex on [0, 7]. This is used as a set of three 1-bit offsets

   Returns a probe index into L.radianceProbeGrid. It may be the *same* index as 
   baseProbeIndex.

   This will wrap in crazy ways when the camera is outside of the bounding box
   of the probes...but that's ok. If that case arises, then the trace is likely to 
   be poor quality anyway. Regardless, this function will still return the index 
   of some valid probe, and that probe can either be used or fail because it does not 
   have visibility to the location desired.

   \see nextCycleIndex, baseProbeIndex
 */
int relativeProbeIndex(in int numProbesPerDimension, in int baseProbeIndex, int relativeIndex) 
{
    // Guaranteed to be a power of 2
    int numProbes = numProbesPerDimension * numProbesPerDimension * numProbesPerDimension;

    ivec3 offset = ivec3(relativeIndex & 1, (relativeIndex >> 1) & 1, (relativeIndex >> 2) & 1);
    ivec3 stride = ivec3(1, numProbesPerDimension, numProbesPerDimension * numProbesPerDimension);

    return (baseProbeIndex + idot(offset, stride)) & (numProbes - 1);
}

/** Given a CycleIndex [0, 7] on a cube of probes, returns the next CycleIndex to use. 

    \see relativeProbeIndex
*/
int nextCycleIndex(int cycleIndex) 
{
    return (cycleIndex + 3) & 7;
}

float square(float x) 
{
    return x * x;
}

vec2 square(vec2 v) 
{
    return v * v;
}

vec3 square(vec3 v) 
{
    return v * v;
}

float squaredLength(vec3 v) 
{
    return dot(v, v);
}

vec3 calculateIndirectIrradiance(
	in vec3 position, 
	in vec3 normal, 
	in vec3 viewDir, 
	in sampler2D collapsedGIIrradianceAtlas, 
	in sampler2D collapsedGIDepthAtlas, 
	in int numProbesPerDimension, 
	in int samplesPerProbeDimension, 
	in vec3 probeStep, 
	in float energyPreservation, 
	in float normalBias)
{
	ivec3 baseGridCoord = baseGridCoord(position, numProbesPerDimension, probeStep);
    vec3 baseProbePos = gridCoordToPosition(baseGridCoord, numProbesPerDimension, probeStep);
	vec3 irradianceSum = vec3(0.0f);
	float weightSum = 0.0f;

	// alpha is how far from the floor(currentVertex) position. on [0, 1] for each axis.
	vec3 alpha = clamp((position - baseProbePos) / probeStep, vec3(0.0f), vec3(1.0f));

	// Iterate over adjacent probe cage
    for (int i = 0; i < 8; ++i) 
	{
        // Compute the offset grid coord and clamp to the probe grid boundary
        // Offset = 0 or 1 along each axis
        ivec3 offset = ivec3(i, i >> 1, i >> 2) & ivec3(1);
        ivec3 probeGridCoord = clamp(baseGridCoord + offset, ivec3(0), ivec3(numProbesPerDimension - 1));
        int p = gridCoordToProbeIndex(probeGridCoord, numProbesPerDimension);

		vec2 atlasOffset = vec2((samplesPerProbeDimension + 2) * p + 1.0f, 1.0f);
		vec2 atlasIrradianceTextureSize = textureSize(collapsedGIIrradianceAtlas, 0);
		vec2 atlasDepthTextureSize = textureSize(collapsedGIDepthAtlas, 0);
		
		// Make cosine falloff in tangent plane with respect to the angle from the surface to the probe so that we never
        // test a probe that is *behind* the surface.
        // It doesn't have to be cosine, but that is efficient to compute and we must clip to the tangent plane.
        vec3 probePos = gridCoordToPosition(probeGridCoord, numProbesPerDimension, probeStep);

		// Bias the position at which visibility is computed; this
        // avoids performing a shadow test *at* a surface, which is a
        // dangerous location because that is exactly the line between
        // shadowed and unshadowed. If the normal bias is too small,
        // there will be light and dark leaks. If it is too large,
        // then samples can pass through thin occluders to the other
        // side (this can only happen if there are MULTIPLE occluders
        // near each other, a wall surface won't pass through itself.)
        vec3 probeToPoint = position - probePos + (normal + 3.0f * viewDir) * normalBias;
        vec3 dir = normalize(-probeToPoint);

		// Compute the trilinear weights based on the grid cell vertex to smoothly
        // transition between probes. Avoid ever going entirely to zero because that
        // will cause problems at the border probes. This isn't really a lerp. 
        // We're using 1-a when offset = 0 and a when offset = 1.
        vec3 trilinear = (1 - offset) * (1.0f - alpha) + offset * alpha;
        float weight = 1.0f;

		// Clamp all of the multiplies. We can't let the weight go to zero because then it would be 
        // possible for *all* weights to be equally low and get normalized
        // up to 1/n. We want to distinguish between weights that are 
        // low because of different factors.

        // Smooth backface test
        {
            // Computed without the biasing applied to the "dir" variable. 
            // This test can cause reflection-map looking errors in the image
            // (stuff looks shiny) if the transition is poor.
            vec3 trueDirectionToProbe = normalize(probePos - position);

            // The naive soft backface weight would ignore a probe when
            // it is behind the surface. That's good for walls. But for small details inside of a
            // room, the normals on the details might rule out all of the probes that have mutual
            // visibility to the point. So, we instead use a "wrap shading" test below inspired by
            // NPR work.
            //weight *= max(0.0001f, dot(trueDirectionToProbe, normal));

            // The small offset at the end reduces the "going to zero" impact
            // where this is really close to exactly opposite
            weight *= square(max(0.0001f, (dot(trueDirectionToProbe, normal) + 1.0f) * 0.5f)) + 0.2f;
        }

		// Moment visibility test
        {
			vec2 localTexelCoord = (float32x3_to_oct(-dir) * 0.5f + 0.5f) * float(samplesPerProbeDimension);
			vec2 probeTexCoord = (localTexelCoord + atlasOffset) / atlasDepthTextureSize;

            float distToProbe = length(probeToPoint);

            vec2 temp = texture(collapsedGIDepthAtlas, probeTexCoord).rg;
            float mean = temp.x;
            float variance = abs(square(temp.x) - temp.y);

            // http://www.punkuser.net/vsm/vsm_paper.pdf; equation 5
            // Need the max in the denominator because biasing can cause a negative displacement
            float chebyshevWeight = variance / (variance + square(max(distToProbe - mean, 0.0f)));
                
            // Increase contrast in the weight 
            chebyshevWeight = max(pow(chebyshevWeight, 3), 0.0f);

            weight *= (distToProbe <= mean) ? 1.0f : chebyshevWeight;
        }

		// Avoid zero weight
        weight = max(0.000001f, weight);
                 
        vec3 irradianceDir = normal;

        vec2 localTexelCoord = (float32x3_to_oct(irradianceDir) * 0.5f + 0.5f) * float(samplesPerProbeDimension);
		vec2 probeTexCoord = (localTexelCoord + atlasOffset) / atlasIrradianceTextureSize;

        vec3 probeIrradiance = texture(collapsedGIIrradianceAtlas, probeTexCoord).rgb;

		// A tiny bit of light is really visible due to log perception, so
        // crush tiny weights but keep the curve continuous. This must be done
        // before the trilinear weights, because those should be preserved.
        const float crushThreshold = 0.2f;
        if (weight < crushThreshold) 
		{
            weight *= weight * weight * (1.0f / square(crushThreshold)); 
        }

		// Trilinear weights
        weight *= trilinear.x * trilinear.y * trilinear.z;

		// Weight in a more-perceptual brightness space instead of radiance space.
        // This softens the transitions between probes with respect to translation.
        // It makes little difference most of the time, but when there are radical transitions
        // between probes this helps soften the ramp.
#       if LINEAR_BLENDING == 0
            probeIrradiance = sqrt(probeIrradiance);
#       endif

		irradianceSum += weight * probeIrradiance;
        weightSum += weight;
	}

	vec3 netIrradiance = irradianceSum / weightSum;

	    // Go back to linear irradiance
#   if LINEAR_BLENDING == 0
        netIrradiance = square(netIrradiance);
#   endif
    netIrradiance *= energyPreservation;

	vec3 lambertianIndirect = 0.5f * M_PI * netIrradiance;
	return lambertianIndirect;
}