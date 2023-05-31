#include "simulation/ElementCommon.h"

#include <cmath>

static int update(UPDATE_FUNC_ARGS);

void Element::Element_ABWM()
{
	Identifier = "RECORDMOD_PT_ABWM";
	Name = "ABWM";
	Colour = PIXPACK(0xFFBBBB);
	MenuVisible = 0;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.0f * CFDS;
	AirLoss = 0.90f;
	Loss = 1.0f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 50;

	HeatConduct = 50;
	Description = "Alaskan Bull Worm. Big, scary, and pink.";

	Properties = TYPE_PART;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
}

static void setSPNG(Particle& abwm, Particle& spng)
{
	float dist = std::hypot(abwm.x - spng.x, abwm.y - spng.y);
	abwm.vx = ((abwm.x - spng.x) / dist) * -2;
	abwm.vy = ((abwm.y - spng.y) / dist) * -2;
	abwm.tmp = spng.x;
	abwm.tmp2 = spng.y;
}

static int update(UPDATE_FUNC_ARGS)
{
	if (sim->elementCount[PT_SPNG])
	{
		int r, r2, rx, ry;
		r = pmap[std::clamp(parts[i].tmp2, 0, YRES - 1)][std::clamp(parts[i].tmp, 0, XRES - 1)];
		if (!r || TYP(r) != PT_SPNG)
		{
			int foundId = -1;
			float foundDist = std::hypot(XRES, YRES) + 1;
			for (int id = 0; id <= sim->parts_lastActiveIndex; id++)
			{
				if (parts[id].type == PT_SPNG)
				{
					float dist = std::hypot(parts[i].x - parts[id].x, parts[i].y - parts[id].y);
					if (dist < foundDist)
					{
						foundId = id;
						foundDist = dist;
					}
				}
			}
			if (foundId != -1)
			{
				setSPNG(parts[i], parts[foundId]);
			}
		}
		else
		{
			for (rx = -2; rx < 3; rx++)
			{
				for (ry = -2; ry < 3; ry++)
				{
					if (rx || ry)
					{
						r2 = pmap[y + ry][x + rx];
						if (r2 && TYP(r2) == PT_SPNG)
						{
							sim->kill_part(ID(r2));
							return 0;
						}
					}
				}
			}
			setSPNG(parts[i], parts[ID(r)]);
		}
	}
	return 0;
}
